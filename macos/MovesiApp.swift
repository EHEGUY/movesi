import SwiftUI
import AppKit

@main
struct MovesiApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate
    
    var body: some Scene {
        // No default WindowGroup scene to keep the app purely in the Menu Bar
        Settings {
            EmptyView()
        }
    }
}

class AppDelegate: NSObject, NSApplicationDelegate {
    var statusController: StatusController?
    
    func applicationDidFinishLaunching(_ notification: Notification) {
        // Set activation policy to accessory so it does not show in the Dock
        NSApp.setActivationPolicy(.accessory)
        
        // Initialize the Menu Bar controller
        statusController = StatusController()
    }
}
