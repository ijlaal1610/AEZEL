plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
    id("org.jetbrains.kotlin.plugin.compose")
}

android {
    namespace = "com.aezel.companion"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.aezel.companion"
        minSdk = 26   // BLE + Jetpack Compose baseline; also matches ESP32-side BLE stack expectations
        targetSdk = 34
        versionCode = 1
        versionName = "0.1.0"
    }

    buildTypes {
        release {
            isMinifyEnabled = false   // enable + add proguard rules once the app is feature-complete, see docs/build.md
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
    }

    buildFeatures {
        compose = true
    }

    packaging {
        resources {
            excludes += "/META-INF/{AL2.0,LGPL2.1}"
        }
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.8.4")
    implementation("androidx.lifecycle:lifecycle-viewmodel-compose:2.8.4")
    implementation("androidx.activity:activity-compose:1.9.1")

    val composeBom = platform("androidx.compose:compose-bom:2024.06.00")
    implementation(composeBom)
    androidTestImplementation(composeBom)
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.ui:ui-graphics")
    implementation("androidx.compose.ui:ui-tooling-preview")
    implementation("androidx.compose.material3:material3")
    implementation("androidx.compose.material:material-icons-extended")
    implementation("androidx.navigation:navigation-compose:2.7.7")

    // Foreground service (BLE connection + notification-forwarding must
    // survive the app being backgrounded while riding) and permission handling.
    implementation("androidx.core:core-splashscreen:1.0.1")

    // No third-party BLE library — Android's own BluetoothGatt API is used
    // directly (see ble/BleConnectionManager.kt). This keeps the dependency
    // surface small and avoids trusting a library's GATT reconnection
    // behavior for something safety-adjacent (this app can send lock/
    // remote-start commands).

    testImplementation("junit:junit:4.13.2")
    // Provides a real org.json.JSONObject implementation for JVM unit
    // tests — without this, Android's org.json classes are unimplemented
    // stubs in a plain `gradle test` run (they throw at runtime), which
    // would make VehicleStateTest exercise nothing real.
    testImplementation("org.json:json:20240303")
    androidTestImplementation("androidx.test.ext:junit:1.2.1")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.6.1")
    debugImplementation("androidx.compose.ui:ui-tooling")
    debugImplementation("androidx.compose.ui:ui-test-manifest")
}
