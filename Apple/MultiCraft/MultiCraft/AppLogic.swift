import AppKit
import Foundation

final class AppLogic: NSObject {
	static let shared = AppLogic()

	func initialize() {
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

public func getUpgrade(key: UnsafePointer<CChar>, extra: UnsafePointer<CChar>) -> Bool {
	AppLogic.log(message: "[EVENT] got upgrade event: \(key)")
	return false
}

public func getSecretKey(key: UnsafePointer<CChar>) -> UnsafePointer<CChar> {
	return ("dummy" as NSString).utf8String!
}

public func finishGame(msg: UnsafePointer<CChar>) -> Never {
	AppLogic.log(message: String(cString: msg))

	DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
		NSApplication.shared.terminate(nil)
	}

	// run the run loop to ensure the app doesn't exit prematurely
	while true {
		RunLoop.current.run(until: Date.distantFuture)
	}
}
