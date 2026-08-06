import Cocoa
import SwiftUI
import ApplicationServices
import Carbon

class StatusController: NSObject {
    private var statusItem: NSStatusItem
    private var popover: NSPopover
    private var timer: Timer?
    private var actionTimer: Timer?
    
    // Application States (shared with SwiftUI)
    @ObservedObject var appState = AppState()
    
    override init() {
        // Create Status Item in Menu Bar
        statusItem = NSStatusBar.system.statusItem(withLength: NSStatusItem.variableLength)
        
        // Create Popover
        popover = NSPopover()
        popover.behavior = .transient
        
        super.init()
        
        // Configure Popover View
        let contentView = ContentView(state: appState, controller: self)
        popover.contentViewController = NSHostingController(rootView: contentView)
        popover.contentSize = NSSize(width: 320, height: 460)
        
        // Configure Status Button
        if let button = statusItem.button {
            button.action = #selector(togglePopover(_:))
            button.target = self
            updateStatusIcon()
        }
        
        // Start 1-second system timer for clock and schedule automation
        timer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            self?.tick()
        }
    }
    
    @objc func togglePopover(_ sender: AnyObject?) {
        if let button = statusItem.button {
            if popover.isShown {
                popover.performClose(sender)
            } else {
                // Refresh accessibility and screen recording check status when opening popover
                appState.checkAccessibility()
                
                popover.show(relativeTo: button.bounds, of: button, preferredEdge: .minY)
                popover.contentViewController?.view.window?.makeKey()
            }
        }
    }
    
    func updateStatusIcon() {
        guard let button = statusItem.button else { return }
        
        // Use SF Symbols on macOS 11+
        if #available(macOS 11.0, *) {
            let imageName = appState.isActive ? "circle.fill" : "circle"
            let config = NSImage.SymbolConfiguration(pointSize: 12, weight: .regular)
            if let image = NSImage(systemSymbolName: imageName, accessibilityDescription: "Movesi")?.withSymbolConfiguration(config) {
                // Style icon colored green when active
                if appState.isActive {
                    button.image = tintImage(image, with: NSColor.systemGreen)
                } else {
                    button.image = image // default template/grey ring
                }
            }
        } else {
            // Fallback for older macOS
            button.title = appState.isActive ? "●" : "○"
        }
    }
    
    // Helper to tint SF Symbols image
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
        } else {
            // Freeze accumulated time active
            if let start = appState.sessionStartTime {
                appState.accumulatedTimeActive += Date().timeIntervalSince(start)
            }
            appState.sessionStartTime = nil
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
                appState.secondsRemaining = appState.sliderInterval
            }
        }
    }
    
    private func checkSchedules() {
        let calendar = Calendar.current
        let now = Date()
        
        let components = calendar.dateComponents([.weekday, .hour], from: now)
        guard let weekday = components.weekday, let hour = components.hour else { return }
        
        // Calendar weekday: 1 = Sunday, 7 = Saturday
        let isWeekend = (weekday == 1 || weekday == 7)
        let isWeekday = !isWeekend
        
        var shouldPause = false
        var shouldResume = false
        
        // 1. Blackout (6pm to 9am, and weekends)
        if appState.enableBlackout {
            let isNight = (hour >= 18 || hour < 9)
            if isWeekend || isNight {
                shouldPause = true
            }
        }
        
        // 2. Work Schedule (9am to 5pm, weekdays only)
        if appState.enableSchedule && !shouldPause {
            let isWorkHours = (hour >= 9 && hour < 17)
            if !isWeekday || !isWorkHours {
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
            updateStatusIcon()
        } else if shouldResume && !appState.isActive && !shouldPause {
            appState.isActive = true
            appState.sessionStartTime = Date()
            appState.secondsRemaining = appState.sliderInterval
            updateStatusIcon()
        }
    }
    
    private func simulateAction() {
        // macOS permissions check
        guard appState.hasAccessibility else {
            print("Action blocked: Accessibility permission required")
            return
        }
        
        // Screen recording is specifically required on Sonoma (14+) for mouse events
        if #available(macOS 14.0, *), !appState.hasScreenRecording {
            print("Action blocked: Screen Recording permission required on Sonoma+")
            return
        }
        
        switch appState.selectedAction {
        case 0: // Mouse Move
            let screenFrame = NSScreen.main?.frame ?? NSRect(x: 0, y: 0, width: 1440, height: 900)
            let currentMousePos = NSEvent.mouseLocation
            
            // Invert Y coordinate for CoreGraphics (0,0 is top-left)
            let currentCGPos = CGPoint(x: currentMousePos.x, y: screenFrame.height - currentMousePos.y)
            
            let dx = CGFloat(Int.random(in: -2...2))
            let dy = CGFloat(Int.random(in: -2...2))
            
            let finalDisplacementX = dx == 0 ? 2 : dx
            let finalDisplacementY = dy == 0 ? 2 : dy
            
            let newPos = CGPoint(x: currentCGPos.x + finalDisplacementX, y: currentCGPos.y + finalDisplacementY)
            
            if let moveEvent = CGEvent(mouseEventSource: nil, mouseType: .mouseMoved, mouseCursorPosition: newPos, mouseButton: .left) {
                moveEvent.post(tap: .cghidEventTap)
            }
            
        case 1: // Mouse Click (Current Location)
            let screenFrame = NSScreen.main?.frame ?? NSRect(x: 0, y: 0, width: 1440, height: 900)
            let currentMousePos = NSEvent.mouseLocation
            let currentCGPos = CGPoint(x: currentMousePos.x, y: screenFrame.height - currentMousePos.y)
            
            if let clickDown = CGEvent(mouseEventSource: nil, mouseType: .leftMouseDown, mouseCursorPosition: currentCGPos, mouseButton: .left),
               let clickUp = CGEvent(mouseEventSource: nil, mouseType: .leftMouseUp, mouseCursorPosition: currentCGPos, mouseButton: .left) {
                clickDown.post(tap: .cghidEventTap)
                clickUp.post(tap: .cghidEventTap)
            }
            
        case 2: // Key Press (F15 - Neutral keycode 113)
            if let keyDown = CGEvent(keyboardEventSource: nil, virtualKey: 113, keyDown: true),
               let keyUp = CGEvent(keyboardEventSource: nil, virtualKey: 113, keyDown: false) {
                keyDown.post(tap: .cghidEventTap)
                keyUp.post(tap: .cghidEventTap)
            }
            
        default:
            break;
        }
    }
}

class AppState: ObservableObject {
    @Published var isActive = false
    @Published var selectedAction = 0 // 0=Move, 1=Click, 2=Key
    @Published var sliderInterval = 30 // 5s to 120s
    @Published var secondsRemaining = 30
    @Published var totalActions = 0
    @Published var sessionStartTime: Date? = nil
    @Published var accumulatedTimeActive: TimeInterval = 0
    @Published var enableSchedule = false
    @Published var enableBlackout = false
    @Published var hasAccessibility = false
    @Published var hasScreenRecording = false
    
    init() {
        checkAccessibility()
    }
    
    func checkAccessibility() {
        // Accessibility permissions check API
        let options = [kAXTrustedCheckOptionPrompt.takeUnretainedValue() as String: false] as CFDictionary
        hasAccessibility = AXIsProcessTrustedWithOptions(options)
        
        // Screen capture preflight check (available on 10.15+)
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
            // Triggers the system Screen Recording permission dialog
            CGRequestScreenCaptureAccess()
        }
    }
}

struct ContentView: View {
    @ObservedObject var state: AppState
    var controller: StatusController
    
    var body: some View {
        VStack(alignment: .leading, spacing: 14) {
            // Header Section
            HStack {
                Text("Movesi")
                    .font(.system(size: 22, weight: .bold))
                Spacer()
                Toggle("", isOn: Binding(
                    get: { state.isActive },
                    set: { _ in controller.toggleSession() }
                ))
                .toggleStyle(SwitchToggleStyle(tint: .green))
            }
            
            // Subtitle Status
            if state.isActive {
                Text("Active • session protected")
                    .font(.subheadline)
                    .foregroundColor(.green)
            } else {
                Text("Paused • session may expire")
                    .font(.subheadline)
                    .foregroundColor(.secondary)
            }
            
            // Accessibility Permission Warning
            if !state.hasAccessibility {
                VStack(alignment: .leading, spacing: 6) {
                    HStack(spacing: 8) {
                        Image(systemName: "exclamationmark.triangle.fill")
                            .foregroundColor(.orange)
                        Text("Accessibility Permission Required")
                            .font(.system(size: 12, weight: .bold))
                            .foregroundColor(.orange)
                    }
                    Text("Accessibility access is needed to simulate user mouse movements and key presses.")
                        .font(.system(size: 10))
                        .foregroundColor(.secondary)
                    Button("Grant Accessibility Access") {
                        state.requestAccessibility()
                    }
                    .font(.system(size: 10, weight: .semibold))
                }
                .padding(10)
                .background(Color(.controlBackgroundColor))
                .cornerRadius(8)
            }
            
            // Screen Recording Permission Warning (macOS Sonoma 14+)
            if state.hasAccessibility && !state.hasScreenRecording {
                VStack(alignment: .leading, spacing: 6) {
                    HStack(spacing: 8) {
                        Image(systemName: "video.fill")
                            .foregroundColor(.orange)
                        Text("Screen Recording Access Required")
                            .font(.system(size: 12, weight: .bold))
                            .foregroundColor(.orange)
                    }
                    Text("macOS 14+ requires Screen Recording access to calculate mouse offsets for event simulation.")
                        .font(.system(size: 10))
                        .foregroundColor(.secondary)
                    Button("Grant Screen Recording Access") {
                        state.requestScreenRecording()
                    }
                    .font(.system(size: 10, weight: .semibold))
                }
                .padding(10)
                .background(Color(.controlBackgroundColor))
                .cornerRadius(8)
            }
            
            Divider()
            
            // Simulation Action selection
            VStack(alignment: .leading, spacing: 6) {
                Text("SIMULATION MODE")
                    .font(.system(size: 10, weight: .semibold))
                    .foregroundColor(.secondary)
                
                Picker("", selection: $state.selectedAction) {
                    Text("Move").tag(0)
                    Text("Click").tag(1)
                    Text("Key").tag(2)
                }
                .pickerStyle(SegmentedPickerStyle())
            }
            
            // Slider Interval & Countdown
            VStack(alignment: .leading, spacing: 6) {
                HStack {
                    Text("INTERVAL")
                        .font(.system(size: 10, weight: .semibold))
                        .foregroundColor(.secondary)
                    Text("Every \(state.sliderInterval)s")
                        .font(.system(size: 11, weight: .bold))
                    Spacer()
                    if state.isActive {
                        Text("Next action: \(state.secondsRemaining)s")
                            .font(.system(size: 11, weight: .bold))
                            .foregroundColor(.green)
                    }
                }
                
                Slider(value: Binding(
                    get: { Double(state.sliderInterval) },
                    set: { val in
                        state.sliderInterval = Int(val)
                        if state.isActive {
                            state.secondsRemaining = Int(val)
                        }
                    }
                ), in: 5...120, step: 1)
            }
            
            // Stats Panel
            VStack(spacing: 8) {
                HStack {
                    VStack(alignment: .leading) {
                        Text("TIME ACTIVE")
                            .font(.system(size: 9, weight: .semibold))
                            .foregroundColor(.secondary)
                        Text(formatDuration(seconds: timeActive()))
                            .font(.system(size: 18, weight: .bold))
                    }
                    Spacer()
                    VStack(alignment: .trailing) {
                        Text("ACTIONS SIM'D")
                            .font(.system(size: 9, weight: .semibold))
                            .foregroundColor(.secondary)
                        Text("\(state.totalActions)")
                            .font(.system(size: 18, weight: .bold))
                    }
                }
            }
            .padding(12)
            .background(Color(.controlBackgroundColor))
            .cornerRadius(10)
            
            Divider()
            
            // Schedule Checks
            VStack(alignment: .leading, spacing: 6) {
                Text("AUTOMATION SETTINGS")
                    .font(.system(size: 10, weight: .semibold))
                    .foregroundColor(.secondary)
                
                Toggle("Work Hours (9am - 5pm, Weekdays)", isOn: $state.enableSchedule)
                    .font(.system(size: 11)
                    )
                Toggle("Blackout (6pm - 9am & Weekends)", isOn: $state.enableBlackout)
                    .font(.system(size: 11))
            }
            
            Spacer()
            
            HStack {
                Spacer()
                Button("Exit") {
                    NSApplication.shared.terminate(nil)
                }
                .font(.system(size: 11))
            }
        }
        .padding(16)
        .frame(width: 320, height: 460)
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
        let s = Int(seconds) % 60
        return String(format: "%02d:%02d:%02d", h, m, s)
    }
}
