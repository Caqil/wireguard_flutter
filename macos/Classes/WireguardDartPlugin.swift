import Cocoa
import FlutterMacOS
import WireGuardKit
import NetworkExtension

public class WireguardDartPlugin: NSObject, FlutterPlugin {

  var bundleId: String?

  public static func register(with registrar: FlutterPluginRegistrar) {
    let channel = FlutterMethodChannel(name: "wireguard_dart", binaryMessenger: registrar.messenger)
    let instance = WireguardDartPlugin()
    registrar.addMethodCallDelegate(instance, channel: channel)
  }

  public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
    switch call.method {
    case "generateKeyPair":
      let privateKey = PrivateKey()
      let privateKeyResponse: [String: Any] = [
          "privateKey": privateKey.base64Key,
          "publicKey": privateKey.publicKey.base64Key,
      ]
      result(privateKeyResponse)
    case "setupTunnel":
      if let args = call.arguments as? Dictionary<String, Any>,
         let argBundleId = args["bundleId"] as? String {
         bundleId = argBundleId
      } else {
        result(FlutterError.init(code: "NATIVE_ERR", message: "required argument: 'bundleId'", details: nil))
        return
      }
      guard let bundleId else {
        result(FlutterError.init(code: "NATIVE_ERR", message: "tunnel not initialized, missing 'bundleId'", details: nil))
        return
      }
      Task {
        do {
          _ = try await setupProviderManager(bundleId: bundleId)
        } catch {
          result(
            FlutterError.init(
              code: "NATIVE_ERR", message: "could not setup VPN tunnel: \(error)", details: nil))
          return
        }
        result("")
      }
    case "connect":
      let cfg: String
      if let args = call.arguments as? Dictionary<String, Any>,
         let argCfg = args["cfg"] as? String {
         cfg = argCfg
      } else {
        result(FlutterError.init(code: "NATIVE_ERR", message: "required argument: 'cfg'", details: nil))
        return
      }
      guard let bundleId else {
        result(FlutterError.init(code: "NATIVE_ERR", message: "tunnel not initialized, missing 'bundleId'", details: nil))
        return
      }
      Task {
        var mgr: NETunnelProviderManager
        do {
          mgr = try await setupProviderManager(bundleId: bundleId)
        } catch {
          result(
            FlutterError.init(
              code: "NATIVE_ERR", message: "could not find VPN tunnel provider: \(error)",
              details: nil))
          return
        }
        do {
          try mgr.connection.startVPNTunnel(options: [
            "cfg": cfg as NSObject
          ])
          result("")
        } catch {
          result(
            FlutterError.init(
              code: "NATIVE_ERR", message: "could not start VPN tunnel: \(error)", details: nil))
        }
      }
    case "disconnect":
      guard let bundleId else {
        result(FlutterError.init(code: "NATIVE_ERR", message: "tunnel not initialized, missing 'bundleId'", details: nil))
        return
      }
      Task {
        var mgr: NETunnelProviderManager
        do {
          mgr = try await setupProviderManager(bundleId: bundleId)
        } catch {
          result(
            FlutterError.init(
              code: "NATIVE_ERR", message: "could not find VPN tunnel provider: \(error)",
              details: nil))
          return
        }
        mgr.connection.stopVPNTunnel()
        result("")
      }
    default:
      result(FlutterMethodNotImplemented)
    }
  }
}

func setupProviderManager(bundleId: String) async throws -> NETunnelProviderManager {
  let mgrs = await fetchManagers()
  let existingMgr = mgrs.first(where: { $0.localizedDescription == "Mysterium VPN" })
  let mgr = existingMgr ?? NETunnelProviderManager()

  mgr.localizedDescription = "Mysterium VPN"
  let proto = NETunnelProviderProtocol()
  proto.providerBundleIdentifier = bundleId
  proto.serverAddress = "127.0.0.1"  // Fake address
  mgr.protocolConfiguration = proto
  mgr.isEnabled = true

  try await saveManager(mgr: mgr)
  return mgr
}

func fetchManagers() async -> [NETunnelProviderManager] {
  return await withCheckedContinuation { continuation in
    NETunnelProviderManager.loadAllFromPreferences { managers, error in
      continuation.resume(returning: (managers ?? []))
    }
  }
}

func saveManager(mgr: NETunnelProviderManager) async throws -> Void {
    return try await withCheckedThrowingContinuation { continuation in
        mgr.saveToPreferences { error in
            if let error {
                continuation.resume(throwing: error)
            } else {
                continuation.resume()
            }
        }
    }
}
