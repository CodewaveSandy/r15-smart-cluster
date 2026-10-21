import Foundation
import CoreBluetooth
import Security

let serviceID = CBUUID(string: "813b1240-8fd9-4d67-a53b-9f0a2531c001")
let metadataID = CBUUID(string: "813b1240-8fd9-4d67-a53b-9f0a2531c002")
let keyQuery: [String: Any] = [kSecClass as String: kSecClassGenericPassword,
    kSecAttrService as String: "dev.r15cluster.preview.trust.v1", kSecAttrAccount as String: "preview"]
if CommandLine.arguments.contains("--check-storage") {
    var query = keyQuery
    query[kSecAttrAccount as String] = "selftest-" + UUID().uuidString
    let sample = Data("R15 temporary storage check".utf8)
    var insertion = query
    insertion[kSecValueData as String] = sample
    insertion[kSecAttrAccessible as String] = kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly
    guard SecItemAdd(insertion as CFDictionary, nil) == errSecSuccess else { exit(1) }
    defer { SecItemDelete(query as CFDictionary) }
    query[kSecReturnData as String] = true
    var item: CFTypeRef?
    guard SecItemCopyMatching(query as CFDictionary, &item) == errSecSuccess,
          let actual = item as? Data, actual == sample else { exit(1) }
    query.removeValue(forKey: kSecReturnData as String)
    guard SecItemDelete(query as CFDictionary) == errSecSuccess else { exit(1) }
    print("Keychain temporary-item round trip passed.")
    exit(0)
}
if CommandLine.arguments.contains("--load") {
    var query = keyQuery; query[kSecReturnData as String] = true
    var item: CFTypeRef?
    let status = SecItemCopyMatching(query as CFDictionary, &item)
    if status == errSecItemNotFound { print("null"); exit(0) }
    guard status == errSecSuccess, let data = item as? Data else { exit(1) }
    FileHandle.standardOutput.write(data); exit(0)
}
if CommandLine.arguments.contains("--save") {
    let data = FileHandle.standardInput.readDataToEndOfFile()
    guard data.count <= 16384 else { exit(1) }
    var status = SecItemUpdate(keyQuery as CFDictionary, [kSecValueData as String: data] as CFDictionary)
    if status == errSecItemNotFound {
        var query = keyQuery; query[kSecValueData as String] = data
        query[kSecAttrAccessible as String] = kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly
        status = SecItemAdd(query as CFDictionary, nil)
    }
    exit(status == errSecSuccess ? 0 : 1)
}

final class Advertiser: NSObject, CBPeripheralManagerDelegate {
    var manager: CBPeripheralManager!
    let metadata: Data
    init(metadata: Data) {
        self.metadata = metadata
        super.init()
        manager = CBPeripheralManager(delegate: self, queue: nil)
    }
    func peripheralManagerDidUpdateState(_ peripheral: CBPeripheralManager) {
        guard peripheral.state == .poweredOn else {
            fputs("R15 discovery: enable Bluetooth and allow R15 Nearby in macOS Privacy settings.\n", stderr)
            return
        }
        peripheral.removeAllServices()
        let characteristic = CBMutableCharacteristic(type: metadataID, properties: [.read], value: metadata, permissions: [.readable])
        let service = CBMutableService(type: serviceID, primary: true)
        service.characteristics = [characteristic]
        peripheral.add(service)
    }
    func peripheralManager(_ peripheral: CBPeripheralManager, didAdd service: CBService, error: Error?) {
        guard error == nil else { fputs("R15 BLE service could not start.\n", stderr); return }
        peripheral.startAdvertising([CBAdvertisementDataServiceUUIDsKey: [serviceID], CBAdvertisementDataLocalNameKey: "R15 Cluster"])
    }
    func peripheralManagerDidStartAdvertising(_ peripheral: CBPeripheralManager, error: Error?) {
        fputs(error == nil ? "R15 nearby BLE advertising active.\n" : "R15 BLE advertising failed.\n", stderr)
    }
}
guard CommandLine.arguments.count == 2,
      let data = try? Data(contentsOf: URL(fileURLWithPath: CommandLine.arguments[1])), data.count <= 512 else { exit(2) }
let advertiser = Advertiser(metadata: data)
// LaunchServices owns this process; stop when its bridge removes the runtime.
let lifetime = Timer.scheduledTimer(withTimeInterval: 1, repeats: true) { _ in
    if !FileManager.default.fileExists(atPath: CommandLine.arguments[1]) { exit(0) }
}
RunLoop.main.run()
