# StudioProd ProGuard / R8 Rules

# Preserve all native JNI methods and NativeBridge
-keepclasseswithmembernames class * {
    native <methods>;
}

-keep class com.example.studioprod.NativeBridge { *; }
-keep interface com.example.studioprod.StudioEngineBridge { *; }

# Preserve Foreground Service
-keep class com.example.studioprod.service.StudioAudioService { *; }

# Preserve Compose Runtime
-keep class androidx.compose.** { *; }
