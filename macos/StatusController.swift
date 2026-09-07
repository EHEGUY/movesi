import Cocoa
import SwiftUI
import ApplicationServices
import Carbon
import IOKit.pwr_mgt
import ServiceManagement

class StatusController: NSObject {
    private var statusItem: NSStatusItem
    private var popover: NSPopover
    private var timer: Timer?
    
    // macOS sleep prevention state
    private var assertionID: IOPMAssertionID = 0
    private var hasAssertion = false
    private var wasActiveBeforeSleep = false
    
    // Shared state
    @ObservedObject var appState = AppState()
    
    override init() {
        statusItem = NSStatusBar.system.statusItem(withLength: NSStatusItem.variableLength)
        
        popover = NSPopover()
        popover.behavior = .transient
        
        super.init()
        
        let contentView = ContentView(state: appState, controller: self)
        popover.contentViewController = NSHostingController(rootView: contentView)
        popover.contentSize = NSSize(width: 320, height: 470)
        
        if let button = statusItem.button {
            button.action = #selector(handleStatusItemClick(_:))
            button.sendAction(on: [.leftMouseUp, .rightMouseUp])
            button.target = self
            updateStatusIcon()
        }
        
        // 1s Timer with 0.5s tolerance to coalesce CPU wakeups
        timer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            self?.tick()
        }
        timer?.tolerance = 0.5
        
        setupSleepNotifications()
    }
    
    deinit {
        allowMacSleep()
        NotificationCenter.default.removeObserver(self)
    }
    
    @objc func handleStatusItemClick(_ sender: AnyObject?) {
        guard let event = NSApp.currentEvent else { return }
        
        if event.type == .rightMouseUp {
            showContextMenu()
        } else {
            togglePopover(sender)
        }
    }
    
    @objc func togglePopover(_ sender: AnyObject?) {
        if let button = statusItem.button {
            if popover.isShown {
                popover.performClose(sender)
            } else {
                appState.checkPermissions()
                popover.show(relativeTo: button.bounds, of: button, preferredEdge: .minY)
                popover.contentViewController?.view.window?.makeKey()
            }
        }
    }

    func closePopover() {
        if popover.isShown {
            popover.performClose(nil)
        }
    }
    
    private func showContextMenu() {
        let menu = NSMenu()
        
        let toggleItem = NSMenuItem(title: appState.isActive ? "Pause Protection" : "Resume Protection", action: #selector(menuToggleProtection), keyEquivalent: "")
        toggleItem.target = self
        if appState.isActive { toggleItem.state = .on }
        menu.addItem(toggleItem)
        
        menu.addItem(NSMenuItem.separator())
        
        let workItem = NSMenuItem(title: "Schedule: Work Hours", action: #selector(menuSetWorkHours), keyEquivalent: "")
        workItem.target = self
        if appState.selectedSchedule == 0 { workItem.state = .on }
        menu.addItem(workItem)
        
        let alwaysItem = NSMenuItem(title: "Schedule: Always On", action: #selector(menuSetAlwaysOn), keyEquivalent: "")
        alwaysItem.target = self
        if appState.selectedSchedule == 1 { alwaysItem.state = .on }
        menu.addItem(alwaysItem)
        
        menu.addItem(NSMenuItem.separator())
        
        let quitItem = NSMenuItem(title: "Quit Movesi", action: #selector(menuQuit), keyEquivalent: "q")
        quitItem.target = self
        menu.addItem(quitItem)
        
        statusItem.menu = menu
        statusItem.button?.performClick(nil)
        statusItem.menu = nil
    }
    
    @objc private func menuToggleProtection() { toggleSession() }
    @objc private func menuSetWorkHours() { appState.selectedSchedule = 0 }
    @objc private func menuSetAlwaysOn() { appState.selectedSchedule = 1 }
    @objc private func menuQuit() { NSApplication.shared.terminate(nil) }
    
    func updateStatusIcon() {
        guard let button = statusItem.button else { return }
        
        let iconName: String
        let tintColor: NSColor
        
        if !appState.hasAccessibility || !appState.hasScreenRecording {
            iconName = "shield.exclamationmark.fill"
            tintColor = .systemOrange
        } else if appState.isActive {
            iconName = "shield.fill"
            tintColor = NSColor(red: 0.06, green: 0.73, blue: 0.51, alpha: 1.0) // Emerald #10B981
        } else {
            iconName = "shield"
            tintColor = NSColor(red: 0.58, green: 0.64, blue: 0.72, alpha: 1.0) // Slate #94A3B8
        }
        
        let config = NSImage.SymbolConfiguration(pointSize: 13, weight: .regular)
        if let image = NSImage(systemSymbolName: iconName, accessibilityDescription: "Movesi")?.withSymbolConfiguration(config) {
            button.image = tintImage(image, with: tintColor)
        }
        
        button.toolTip = appState.isActive ? "Movesi - Next action in \(appState.secondsRemaining)s" : "Movesi - Paused"
    }
    
    private func tintImage(_ image: NSImage, with color: NSColor) -> NSImage {
        guard let tinted = image.copy() as? NSImage else { return image }
        tinted.lockFocus()
        color.set()
        let imageRect = NSRect(origin: .zero, size: tinted.size)
        imageRect.fill(using: .sourceAtop)
        tinted.unlockFocus()
        tinted.isTemplate = false
        return tinted
    }
    
    func toggleSession() {
        appState.isActive.toggle()
        if appState.isActive {
            appState.sessionStartTime = Date()
            appState.secondsRemaining = appState.sliderInterval
            preventMacSleep()
        } else {
            if let start = appState.sessionStartTime {
                appState.accumulatedTimeActive += Date().timeIntervalSince(start)
            }
            appState.sessionStartTime = nil
            allowMacSleep()
        }
        updateStatusIcon()
    }
    
    private func tick() {
        checkSchedules()
        
        if appState.isActive {
            appState.secondsRemaining -= 1
            if appState.secondsRemaining <= 0 {
                simulateAction()
                appState.totalActions += 1
                
                // Jittered interval (±30%)
                let jitter = Double.random(in: 0.7...1.3)
                appState.secondsRemaining = max(10, Int(Double(appState.sliderInterval) * jitter))
            }
        }
        updateStatusIcon()
    }
    
    private func checkSchedules() {
        guard appState.selectedSchedule != 1 else { return } // Always On
        
        let calendar = Calendar.current
        let now = Date()
        let components = calendar.dateComponents([.weekday, .hour], from: now)
        guard let weekday = components.weekday, let hour = components.hour else { return }
        
        let isWeekend = (weekday == 1 || weekday == 7)
        var shouldPause = false
        var shouldResume = false
        
        if appState.selectedSchedule == 0 { // Work Hours
            let isWorkHours = (hour >= 9 && hour < 17)
            if isWeekend || !isWorkHours {
                shouldPause = true
            } else {
                shouldResume = true
            }
        } else if appState.selectedSchedule == 2 { // Custom
            let inRange = (hour >= appState.customStartHour && hour < appState.customEndHour)
            if !inRange {
                shouldPause = true
            } else {
                shouldResume = true
            }
        }
        
        if shouldPause && appState.isActive {
            appState.isActive = false
            if let start = appState.sessionStartTime {
                appState.accumulatedTimeActive += Date().timeIntervalSince(start)
            }
            appState.sessionStartTime = nil
            allowMacSleep()
            updateStatusIcon()
        } else if shouldResume && !appState.isActive && !shouldPause {
            appState.isActive = true
            appState.sessionStartTime = Date()
            appState.secondsRemaining = appState.sliderInterval
            preventMacSleep()
            updateStatusIcon()
        }
    }
    
    private func setupSleepNotifications() {
        let ws = NSWorkspace.shared.notificationCenter
        ws.addObserver(self, selector: #selector(handleSleep), name: NSWorkspace.willSleepNotification, object: nil)
        ws.addObserver(self, selector: #selector(handleSleep), name: NSWorkspace.screensaverDidStartNotification, object: nil)
        ws.addObserver(self, selector: #selector(handleWake), name: NSWorkspace.didWakeNotification, object: nil)
        ws.addObserver(self, selector: #selector(handleWake), name: NSWorkspace.screensaverDidStopNotification, object: nil)
    }
    
    @objc private func handleSleep() {
        if appState.isActive {
            wasActiveBeforeSleep = true
            appState.isActive = false
            allowMacSleep()
            updateStatusIcon()
        }
    }
    
    @objc private func handleWake() {
        if wasActiveBeforeSleep {
            wasActiveBeforeSleep = false
            appState.isActive = true
            preventMacSleep()
            updateStatusIcon()
        }
    }
    
    private func preventMacSleep() {
        guard !hasAssertion else { return }
        let reason = "Movesi Activity Simulation" as CFString
        let result = IOPMAssertionCreateWithName(
            kIOPMAssertionTypeNoDisplaySleep as CFString,
            IOPMAssertionLevel(kIOPMAssertionLevelOn),
            reason,
            &assertionID
        )
        if result == kIOReturnSuccess {
            hasAssertion = true
        }
    }
    
    private func allowMacSleep() {
        guard hasAssertion else { return }
        let result = IOPMAssertionRelease(assertionID)
        if result == kIOReturnSuccess {
            hasAssertion = false
            assertionID = 0
        }
    }
    
    private func simulateAction() {
        guard appState.hasAccessibility else { return }
        
        switch appState.selectedAction {
        case 0: // Mouse Move
            let screenFrame = NSScreen.main?.frame ?? NSRect(x: 0, y: 0, width: 1440, height: 900)
            let currentMousePos = NSEvent.mouseLocation
            let currentCGPos = CGPoint(x: currentMousePos.x, y: screenFrame.height - currentMousePos.y)
            
            let dx = CGFloat(Int.random(in: 10...20)) * (Bool.random() ? 1 : -1)
            let dy = CGFloat(Int.random(in: 10...20)) * (Bool.random() ? 1 : -1)
            let newPos = CGPoint(x: currentCGPos.x + dx, y: currentCGPos.y + dy)
            
            if let moveEvent = CGEvent(mouseEventSource: nil, mouseType: .mouseMoved, mouseCursorPosition: newPos, mouseButton: .left) {
                moveEvent.post(tap: .cghidEventTap)
            }
            
        case 1: // Key Press (F15 / Virtual key 113)
            let keycode = CGKeyCode(113)
            if let keyDown = CGEvent(keyboardEventSource: nil, virtualKey: keycode, keyDown: true),
               let keyUp = CGEvent(keyboardEventSource: nil, virtualKey: keycode, keyDown: false) {
                keyDown.post(tap: .cghidEventTap)
                keyUp.post(tap: .cghidEventTap)
            }
            
        case 2: // Mouse Click
            let screenFrame = NSScreen.main?.frame ?? NSRect(x: 0, y: 0, width: 1440, height: 900)
            let currentMousePos = NSEvent.mouseLocation
            let currentCGPos = CGPoint(x: currentMousePos.x, y: screenFrame.height - currentMousePos.y)
            
            if let clickDown = CGEvent(mouseEventSource: nil, mouseType: .leftMouseDown, mouseCursorPosition: currentCGPos, mouseButton: .left),
               let clickUp = CGEvent(mouseEventSource: nil, mouseType: .leftMouseUp, mouseCursorPosition: currentCGPos, mouseButton: .left) {
                clickDown.post(tap: .cghidEventTap)
                clickUp.post(tap: .cghidEventTap)
            }
            
        default:
            break
        }
    }
}

class AppState: ObservableObject {
    @Published var isActive = false
    @Published var selectedAction: Int {
        didSet { UserDefaults.standard.set(selectedAction, forKey: "selectedAction") }
    }
    @Published var selectedSchedule: Int {
        didSet { UserDefaults.standard.set(selectedSchedule, forKey: "selectedSchedule") }
    }
    @Published var sliderInterval: Int {
        didSet { UserDefaults.standard.set(sliderInterval, forKey: "sliderInterval") }
    }
    @Published var customStartHour: Int {
        didSet { UserDefaults.standard.set(customStartHour, forKey: "customStartHour") }
    }
    @Published var customEndHour: Int {
        didSet { UserDefaults.standard.set(customEndHour, forKey: "customEndHour") }
    }
    @Published var secondsRemaining = 45
    @Published var totalActions: Int {
        didSet { UserDefaults.standard.set(totalActions, forKey: "totalActions") }
    }
    @Published var accumulatedTimeActive: TimeInterval {
        didSet { UserDefaults.standard.set(accumulatedTimeActive, forKey: "accumulatedTimeActive") }
    }
    @Published var launchAtLogin: Bool {
        didSet { updateLaunchAtLogin(launchAtLogin) }
    }
    
    @Published var sessionStartTime: Date? = nil
    @Published var hasAccessibility = false
    @Published var hasScreenRecording = false
    @Published var hasCompletedOnboarding: Bool {
        didSet { UserDefaults.standard.set(hasCompletedOnboarding, forKey: "hasCompletedOnboarding") }
    }
    
    init() {
        self.selectedAction = UserDefaults.standard.integer(forKey: "selectedAction")
        self.selectedSchedule = UserDefaults.standard.object(forKey: "selectedSchedule") != nil ? UserDefaults.standard.integer(forKey: "selectedSchedule") : 1
        self.sliderInterval = UserDefaults.standard.integer(forKey: "sliderInterval") > 0 ? UserDefaults.standard.integer(forKey: "sliderInterval") : 45
        self.customStartHour = UserDefaults.standard.integer(forKey: "customStartHour") > 0 ? UserDefaults.standard.integer(forKey: "customStartHour") : 9
        self.customEndHour = UserDefaults.standard.integer(forKey: "customEndHour") > 0 ? UserDefaults.standard.integer(forKey: "customEndHour") : 17
        self.totalActions = UserDefaults.standard.integer(forKey: "totalActions")
        self.accumulatedTimeActive = UserDefaults.standard.double(forKey: "accumulatedTimeActive")
        self.hasCompletedOnboarding = UserDefaults.standard.bool(forKey: "hasCompletedOnboarding")
        self.launchAtLogin = UserDefaults.standard.bool(forKey: "launchAtLogin")
        
        checkPermissions()
    }
    
    func checkPermissions() {
        let options = [kAXTrustedCheckOptionPrompt.takeUnretainedValue() as String: false] as CFDictionary
        hasAccessibility = AXIsProcessTrustedWithOptions(options)
        
        if #available(macOS 10.15, *) {
            hasScreenRecording = CGPreflightScreenCaptureAccess()
        } else {
            hasScreenRecording = true
        }
    }
    
    func requestAccessibility() {
        let options = [kAXTrustedCheckOptionPrompt.takeUnretainedValue() as String: true] as CFDictionary
        _ = AXIsProcessTrustedWithOptions(options)
    }
    
    func requestScreenRecording() {
        if #available(macOS 10.15, *) {
            CGRequestScreenCaptureAccess()
        }
    }
    
    private func updateLaunchAtLogin(_ enabled: Bool) {
        UserDefaults.standard.set(enabled, forKey: "launchAtLogin")
        if #available(macOS 13.0, *) {
            do {
                if enabled {
                    try SMAppService.mainApp.register()
                } else {
                    try SMAppService.mainApp.unregister()
                }
            } catch {
                print("SMAppService launch at login failed: \(error)")
            }
        }
    }
}

// SwiftUI Popover Content View
struct ContentView: View {
    @ObservedObject var state: AppState
    var controller: StatusController
    
    var body: some View {
        Group {
            if !state.hasCompletedOnboarding && (!state.hasAccessibility || !state.hasScreenRecording) {
                OnboardingView(state: state)
            } else {
                MainView(state: state, controller: controller)
            }
        }
        .frame(width: 320, height: 470)
        .background(Color(NSColor.windowBackgroundColor))
    }
}

struct MainView: View {
    @ObservedObject var state: AppState
    var controller: StatusController
    
    let emerald = Color(red: 0.06, green: 0.73, blue: 0.51)
    let slate = Color(red: 0.58, green: 0.64, blue: 0.72)
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            // 1. Header (~56px)
            HStack {
                Text("Movesi")
                    .font(.system(size: 13, weight: .medium))
                    .foregroundColor(.secondary)
                Spacer()
                Button(action: { controller.closePopover() }) {
                    Image(systemName: "xmark")
                        .font(.system(size: 11, weight: .bold))
                        .foregroundColor(.secondary)
                        .padding(4)
                        .background(Circle().fill(Color.secondary.opacity(0.15)))
                }
                .buttonStyle(PlainButtonStyle())
            }
            .padding(.top, 4)
            
            // 2. Status Card (~160px)
            VStack(alignment: .leading, spacing: 10) {
                HStack(spacing: 10) {
                    Circle()
                        .fill(state.isActive ? emerald : slate)
                        .frame(width: 10, height: 10)
                        .scaleEffect(state.isActive ? 1.2 : 1.0)
                        .animation(state.isActive ? Animation.easeInOut(duration: 0.8).repeatForever(autoreverses: true) : .default, value: state.isActive)
                    
                    Text(state.isActive ? "Active" : "Paused")
                        .font(.system(size: 22, weight: .semibold))
                        .foregroundColor(state.isActive ? emerald : .primary)
                }
                
                Text(state.isActive ? "Simulating activity every ~\(state.sliderInterval)s" : "Protection paused")
                    .font(.system(size: 11))
                    .foregroundColor(.secondary)
                
                Button(action: { controller.toggleSession() }) {
                    Text(state.isActive ? "Pause Protection" : "Resume Protection")
                        .font(.system(size: 13, weight: .semibold))
                        .foregroundColor(state.isActive ? .primary : .white)
                        .frame(maxWidth: .infinity)
                        .frame(height: 44)
                        .background(state.isActive ? Color.secondary.opacity(0.2) : emerald)
                        .cornerRadius(22)
                }
                .buttonStyle(PlainButtonStyle())
            }
            .padding(14)
            .background(Color(NSColor.controlBackgroundColor))
            .cornerRadius(12)
            
            // 3. Stats Grid (2x2)
            LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())], spacing: 8) {
                StatChip(title: "Session Time", value: formatDuration(seconds: timeActive()))
                StatChip(title: "Actions Sent", value: "\(state.totalActions)")
                StatChip(title: "Next Action", value: state.isActive ? "in \(state.secondsRemaining)s" : "Paused", valueColor: state.isActive ? emerald : .primary)
                StatChip(title: "Schedule", value: scheduleName())
            }
            
            Divider()
            
            // Action Selector (3 Pills)
            VStack(alignment: .leading, spacing: 4) {
                Text("ACTION TYPE")
                    .font(.system(size: 11, weight: .medium))
                    .foregroundColor(.secondary)
                
                HStack(spacing: 6) {
                    PillButton(title: "Mouse Move", isSelected: state.selectedAction == 0) { state.selectedAction = 0 }
                    PillButton(title: "Key Press", isSelected: state.selectedAction == 1) { state.selectedAction = 1 }
                    PillButton(title: "Mouse Click", isSelected: state.selectedAction == 2) { state.selectedAction = 2 }
                }
            }
            
            // Schedule Selector (3 Pills)
            VStack(alignment: .leading, spacing: 4) {
                Text("SCHEDULE")
                    .font(.system(size: 11, weight: .medium))
                    .foregroundColor(.secondary)
                
                HStack(spacing: 6) {
                    PillButton(title: "Work Hours", isSelected: state.selectedSchedule == 0) { state.selectedSchedule = 0 }
                    PillButton(title: "Always On", isSelected: state.selectedSchedule == 1) { state.selectedSchedule = 1 }
                    PillButton(title: "Custom", isSelected: state.selectedSchedule == 2) { state.selectedSchedule = 2 }
                }
            }
            
            // Slider (Slower / Faster)
            VStack(spacing: 2) {
                HStack {
                    Text("Slower").font(.system(size: 11)).foregroundColor(.secondary)
                    Spacer()
                    Text("Faster").font(.system(size: 11)).foregroundColor(.secondary)
                }
                Slider(value: Binding(
                    get: { Double(state.sliderInterval) },
                    set: { val in
                        state.sliderInterval = Int(val)
                        if state.isActive { state.secondsRemaining = Int(val) }
                    }
                ), in: 15...120)
                .accentColor(emerald)
            }
            
            // Footer: Launch at Login Checkbox
            Toggle("Launch at login", isOn: $state.launchAtLogin)
                .font(.system(size: 11))
                .toggleStyle(CheckboxToggleStyle())
        }
        .padding(16)
    }
    
    private func timeActive() -> TimeInterval {
        var total = state.accumulatedTimeActive
        if state.isActive, let start = state.sessionStartTime {
            total += Date().timeIntervalSince(start)
        }
        return total
    }
    
    private func formatDuration(seconds: TimeInterval) -> String {
        let h = Int(seconds) / 3600
        let m = (Int(seconds) % 3600) / 60
        return "\(h)h \(m)m"
    }
    
    private func scheduleName() -> String {
        switch state.selectedSchedule {
        case 0: return "Work Hours"
        case 1: return "Always On"
        default: return "Custom"
        }
    }
}

struct StatChip: View {
    let title: String
    let value: String
    var valueColor: Color = .primary
    
    var body: some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(title)
                .font(.system(size: 11))
                .foregroundColor(.secondary)
            Text(value)
                .font(.system(size: 13, weight: .bold))
                .foregroundColor(valueColor)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(8)
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(8)
    }
}

struct PillButton: View {
    let title: String
    let isSelected: Bool
    let action: () -> Void
    
    let emerald = Color(red: 0.06, green: 0.73, blue: 0.51)
    
    var body: some View {
        Button(action: action) {
            Text(title)
                .font(.system(size: 11, weight: isSelected ? .semibold : .regular))
                .foregroundColor(isSelected ? .white : .primary)
                .frame(maxWidth: .infinity)
                .frame(height: 24)
                .background(isSelected ? emerald : Color.secondary.opacity(0.12))
                .cornerRadius(4)
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// 2-Step Permissions Onboarding View
struct OnboardingView: View {
    @ObservedObject var state: AppState
    @State private var currentStep = 1
    let emerald = Color(red: 0.06, green: 0.73, blue: 0.51)
    
    var body: some View {
        VStack(spacing: 20) {
            Text("Permissions Required")
                .font(.system(size: 15, weight: .bold))
            
            if currentStep == 1 {
                VStack(spacing: 12) {
                    Image(systemName: "hand.tap.fill")
                        .font(.system(size: 40))
                        .foregroundColor(emerald)
                    Text("Step 1: Accessibility Access")
                        .font(.system(size: 13, weight: .semibold))
                    Text("Movesi requires Accessibility permission to send hardware-level mouse movements and keep your system active.")
                        .font(.system(size: 11))
                        .foregroundColor(.secondary)
                        .multilineTextAlignment(.center)
                    
                    Button("Grant →") {
                        state.requestAccessibility()
                        currentStep = 2
                    }
                    .font(.system(size: 13, weight: .bold))
                    .foregroundColor(.white)
                    .frame(width: 140, height: 36)
                    .background(emerald)
                    .cornerRadius(4)
                    .buttonStyle(PlainButtonStyle())
                }
            } else {
                VStack(spacing: 12) {
                    Image(systemName: "rectangle.dashed.badge.record")
                        .font(.system(size: 40))
                        .foregroundColor(emerald)
                    Text("Step 2: Screen Recording Access")
                        .font(.system(size: 13, weight: .semibold))
                    Text("macOS 14+ requires Screen Recording permission for input events to bypass display sleep.")
                        .font(.system(size: 11))
                        .foregroundColor(.secondary)
                        .multilineTextAlignment(.center)
                    
                    Button("Grant →") {
                        state.requestScreenRecording()
                        state.hasCompletedOnboarding = true
                    }
                    .font(.system(size: 13, weight: .bold))
                    .foregroundColor(.white)
                    .frame(width: 140, height: 36)
                    .background(emerald)
                    .cornerRadius(4)
                    .buttonStyle(PlainButtonStyle())
                }
            }
        }
        .padding(24)
    }
}
