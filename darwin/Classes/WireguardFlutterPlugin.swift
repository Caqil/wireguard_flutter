#if os(iOS)
import Flutter
import UIKit
#elseif os(macOS)
import FlutterMacOS
import Cocoa
#else
#error("Unsupported platform")
#endif

import NetworkExtension

public class WireguardFlutterPlugin: NSObject, FlutterPlugin {
    private static var utils : VPNUtils! = VPNUtils()
     
    public static var stage: FlutterEventSink?
    private var initialized : Bool = false
    var wireguardMethodChannel: FlutterMethodChannel?
    
    public static func register(with registrar: FlutterPluginRegistrar) {
        let instance = WireguardFlutterPlugin()
        instance.onRegister(registrar)
    }
    
    public func onRegister(_ registrar: FlutterPluginRegistrar){
        #if os(iOS)
        let messenger = registrar.messenger()
        #else
        let messenger = registrar.messenger
        #endif
        let wireguardMethodChannel = FlutterMethodChannel(name: "billion.group.wireguard_flutter/wgcontrol", binaryMessenger: messenger)
        let vpnStageE = FlutterEventChannel(name: "billion.group.wireguard_flutter/wgstage", binaryMessenger: messenger)
        
        vpnStageE.setStreamHandler(VPNConnectionHandler())
        wireguardMethodChannel.setMethodCallHandler({(call: FlutterMethodCall, result: @escaping FlutterResult) -> Void in
            switch call.method {
            case "stage":
                result(WireguardFlutterPlugin.utils.currentStatus())
            case "initialize":
                let localizedDescription: String? = (call.arguments as? [String: Any])?["localizedDescription"] as? String
                if localizedDescription == nil {
                    result(FlutterError(code: "-3", message: "localizedDescription content empty or null", details: nil))
                    return
                }
                WireguardFlutterPlugin.utils.localizedDescription = localizedDescription
                WireguardFlutterPlugin.utils.loadProviderManager { (err: Error?) in
                    if err == nil {
                        result(WireguardFlutterPlugin.utils.currentStatus())
                    } else {
                        result(FlutterError(code: "-4", message: err.debugDescription, details: err?.localizedDescription))
                    }
                }
                self.initialized = true
            case "stop":
                self.disconnect(result: result)
            case "start":
                let serverAddress: String? = (call.arguments as? [String: Any])?["serverAddress"] as? String
                let wgQuickConfig: String? = (call.arguments as? [String: Any])?["wgQuickConfig"] as? String
                let providerBundleIdentifier: String? = (call.arguments as? [String: Any])?["providerBundleIdentifier"] as? String
                self.connect(serverAddress: serverAddress!, wgQuickConfig: wgQuickConfig!, providerBundleIdentifier: providerBundleIdentifier!, result: result)
            
            case "dispose":
                self.initialized = false
            default:
                break
            }
        })
    }
    
    private func connect(serverAddress: String, wgQuickConfig: String, providerBundleIdentifier: String, result: @escaping FlutterResult) {
        WireguardFlutterPlugin.utils.configureVPN(serverAddress: serverAddress, wgQuickConfig: wgQuickConfig, providerBundleIdentifier: providerBundleIdentifier) { success in
            result(success)
        }
    }
    
    private func disconnect(result: @escaping FlutterResult) {
        WireguardFlutterPlugin.utils.stopVPN() { success in
            result(success)
        }
    }
    
    private func getDownloadData(result: @escaping FlutterResult) {
        WireguardFlutterPlugin.utils.getTransferData { downloadData, _ in
            result(downloadData)
        }
    }
    
    private func getUploadData(result: @escaping FlutterResult) {
        WireguardFlutterPlugin.utils.getTransferData { _, uploadData in
            result(uploadData)
        }
    }
    
    class VPNConnectionHandler: NSObject, FlutterStreamHandler {
        private var vpnConnection: FlutterEventSink?
        private var vpnConnectionObserver: NSObjectProtocol?
        
        func onListen(withArguments arguments: Any?, eventSink events: @escaping FlutterEventSink) -> FlutterError? {
            if let observer = vpnConnectionObserver {
                NotificationCenter.default.removeObserver(observer)
            }
            
            vpnConnectionObserver = NotificationCenter.default.addObserver(forName: NSNotification.Name.NEVPNStatusDidChange, object: nil, queue: nil) { [weak self] notification in
                guard let self = self, let connection = self.vpnConnection else {
                    return
                }
                
                let nevpnconn = notification.object as! NEVPNConnection
                let status = nevpnconn.status
                
                connection(WireguardFlutterPlugin.utils.onVpnStatusChangedString(notification: status))
            }
            
            self.vpnConnection = events
            
            NETunnelProviderManager.loadAllFromPreferences { managers, error in
                events(WireguardFlutterPlugin.utils.onVpnStatusChangedString(notification: managers?.first?.connection.status))
            }
            
            return nil
        }
        
        func onCancel(withArguments arguments: Any?) -> FlutterError? {
            if let observer = vpnConnectionObserver {
                NotificationCenter.default.removeObserver(observer)
            }
            vpnConnection = nil
            
            return nil
        }
    }
}

@available(iOS 15.0, *)
class VPNUtils {
    var providerManager: NETunnelProviderManager!
    var providerBundleIdentifier: String?
    var localizedDescription: String?
    var groupIdentifier: String?
    var serverAddress: String?
    var stage: FlutterEventSink!
    
    func loadProviderManager(completion: @escaping (_ error: Error?) -> Void) {
        NETunnelProviderManager.loadAllFromPreferences { (managers, error) in
            if error == nil {
                self.providerManager = managers?.first ?? NETunnelProviderManager()
                completion(nil)
            } else {
                completion(error)
            }
        }
    }
    
    func onVpnStatusChanged(notification: NEVPNStatus) {
        switch notification {
        case .connected:
            stage?("connected")
        case .connecting:
            stage?("connecting")
        case .disconnected:
            stage?("disconnected")
        case .disconnecting:
            stage?("disconnecting")
        case .invalid:
            stage?("invalid")
        case .reasserting:
            stage?("reasserting")
        @unknown default:
            stage?("disconnected")
        }
    }
    
    func onVpnStatusChangedString(notification: NEVPNStatus?) -> String? {
        if notification == nil {
            return "disconnected"
        }
        switch notification! {
        case .connected:
            return "connected"
        case .connecting:
            return "connecting"
        case .disconnected:
            return "disconnected"
        case .disconnecting:
            return "disconnecting"
        case .invalid:
            return "invalid"
        case .reasserting:
            return "reasserting"
        @unknown default:
            return "disconnected"
        }
    }
    
    func currentStatus() -> String? {
        if self.providerManager != nil {
            return onVpnStatusChangedString(notification: self.providerManager.connection.status)
        } else {
            return "disconnected"
        }
    }
    
    func configureVPN(serverAddress: String?, wgQuickConfig: String?, providerBundleIdentifier: String?, completion: @escaping (Bool) -> Void) {
        NETunnelProviderManager.loadAllFromPreferences { tunnelManagersInSettings, error in
            if let error = error {
                NSLog("Error (loadAllFromPreferences): \(error)")
                completion(false)
                return
            }
            let preExistingTunnelManager = tunnelManagersInSettings?.first
            let tunnelManager = preExistingTunnelManager ?? NETunnelProviderManager()
            
            let protocolConfiguration = NETunnelProviderProtocol()
            
            protocolConfiguration.providerBundleIdentifier = providerBundleIdentifier!
            protocolConfiguration.serverAddress = serverAddress
            protocolConfiguration.providerConfiguration = ["wgQuickConfig": wgQuickConfig!]
            
            tunnelManager.protocolConfiguration = protocolConfiguration
            tunnelManager.isEnabled = true
            
            tunnelManager.saveToPreferences { error in
                if let error = error {
                    NSLog("Error (saveToPreferences): \(error)")
                    completion(false)
                } else {
                    tunnelManager.loadFromPreferences { error in
                        if let error = error {
                            NSLog("Error (loadFromPreferences): \(error)")
                            completion(false)
                        } else {
                            NSLog("Starting the tunnel")
                            if let session = tunnelManager.connection as? NETunnelProviderSession {
                                do {
                                    try session.startTunnel(options: nil)
                                    completion(true)
                                } catch {
                                    NSLog("Error (startTunnel): \(error)")
                                    completion(false)
                                }
                            } else {
                                NSLog("tunnelManager.connection is invalid")
                                completion(false)
                            }
                        }
                    }
                }
            }
        }
    }
    
    func stopVPN(completion: @escaping (Bool?) -> Void) {
        NETunnelProviderManager.loadAllFromPreferences { tunnelManagersInSettings, error in
            if let error = error {
                NSLog("Error (loadAllFromPreferences): \(error)")
                completion(false)
                return
            }
            
            if let tunnelManager = tunnelManagersInSettings?.first {
                guard let session = tunnelManager.connection as? NETunnelProviderSession else {
                    NSLog("tunnelManager.connection is invalid")
                    completion(false)
                    return
                }
                switch session.status {
                case .connected, .connecting, .reasserting:
                    NSLog("Stopping the tunnel")
                    session.stopTunnel()
                    completion(true)
                default:
                    completion(false)
                }
            } else {
                completion(false)
            }
        }
    }
    
    func getTransferData(completion: @escaping (UInt64?, UInt64?) -> Void) {
        NETunnelProviderManager.loadAllFromPreferences { managers, error in
            guard let manager = managers?.first else {
                completion(nil, nil)
                return
            }
            
            guard let session = manager.connection as? NETunnelProviderSession else {
                completion(nil, nil)
                return
            }
            
            do {
                try session.sendProviderMessage("getTransferData".data(using: .utf8)!) { response in
                    guard let response = response, response.count == 16 else {
                        completion(nil, nil)
                        return
                    }
                    
                    let downloadData = response.withUnsafeBytes { $0.load(as: UInt64.self) }
                    let uploadData = response.advanced(by: 8).withUnsafeBytes { $0.load(as: UInt64.self) }
                    completion(downloadData, uploadData)
                }
            } catch {
                NSLog("Error (sendProviderMessage): \(error)")
                completion(nil, nil)
            }
        }
    }

}
