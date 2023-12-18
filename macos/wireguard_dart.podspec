#
# To learn more about a Podspec see http://guides.cocoapods.org/syntax/podspec.html.
# Run `pod lib lint wireguard_dart.podspec` to validate before publishing.
#
Pod::Spec.new do |s|
  s.name = "wireguard_dart"
  s.version = "0.0.1"
  s.summary = "Wireguard (macOS)"
  s.description = <<-DESC
Wireguard Dart SDK for macOS
                       DESC
  s.homepage = "https://github.com/mysteriumnetwork/wireguard_dart"
  s.license = { :file => "../LICENSE" }
  s.author = { "Mysterium Network" => "mysterium-dev@mysterium.network" }

  s.source = { :path => "." }
  s.source_files = "Classes/**/*"

  s.platform = :osx, "12.0"

  s.pod_target_xcconfig = { "DEFINES_MODULE" => "YES" }
  s.swift_version = "5.7"

  s.dependency "FlutterMacOS"
  s.dependency "WireGuardKit"
end
