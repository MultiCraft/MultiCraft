package com.bugsnag.android;

import android.app.Application;
import android.util.Log;

public class Bugsnag {
	public static void notify(Throwable e) {
		Log.getStackTraceString(e);
	}

	public static void leaveBreadcrumb(String s) {
		Log.d("Bugsnag", s);
	}

	public static void start(Application application) {
		Log.d("Bugsnag", "Bugsnag initialized");
	}
}
