import AppKit
import Foundation

final class AppLogic: NSObject {
	static let shared = AppLogic()

	public func initialize() {
		Self.log(message: "[INIT] initialized")
	}

	static func log(message: String, object: Any? = nil, function: String = #function) {
		#if DEBUG
		print("\(Date()) [\(function)] - \(message) \(object ?? "")")
		#endif
	}
}

public func initApple() {
	AppLogic.shared.initialize()
}

public func getScreenScale() -> Float {
	let scale = NSScreen.main?.backingScaleFactor ?? 1.0
	return Float(scale)
}

public func getUpgrade(key: UnsafePointer<CChar>) {
	AppLogic.log(message: "[EVENT] got upgrade event: \(key)")
}

public func getSecretKey(key: UnsafePointer<CChar>) -> UnsafePointer<CChar> {
	return ("dummy" as NSString).utf8String!
}
