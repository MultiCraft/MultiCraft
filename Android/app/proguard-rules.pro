# Add project specific ProGuard rules here.
# You can control the set of applied configuration files using the
# proguardFiles setting in build.gradle.kts.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# If your project uses WebView with JS, uncomment the following
# and specify the fully qualified class name to the JavaScript interface
# class:
#-keepclassmembers class fqcn.of.javascript.interface.for.webview {
#   public *;
#}

# Uncomment this to preserve the line number information for
# debugging stack traces.
#-keepattributes SourceFile,LineNumberTable

# If you keep the line number information, uncomment this to
# hide the original source file name.
#-renamesourcefileattribute SourceFile
-keep class com.multicraft.game.GameActivity {
	public <methods>;
	#noinspection ShrinkerUnresolvedReference
	void showDialog(java.lang.String, java.lang.String, int);
	boolean isDialogActive();
	java.lang.String getDialogValue();
	float getDensity();
	void notifyServerConnect(boolean);
	void notifyExitGame();
	boolean openURI(java.lang.String, boolean);
	void finishGame(java.lang.String);
	void handleError(java.lang.String);
	boolean upgrade(java.lang.String, java.lang.String);
	java.lang.String getSecretKey(java.lang.String);
	void hideSplashScreen();
	boolean needsExtractAssets();
	void vibrationEffect(int, int);
}
-keepclasseswithmembernames,includedescriptorclasses class * {
	native <methods>;
}
-keepclassmembers class androidx.lifecycle.ReportFragment$** { *; }
# SDL
-keep public class org.libsdl.app.** { public *; }
