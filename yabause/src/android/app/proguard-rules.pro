-keepattributes *Annotation*
-keepattributes SourceFile,LineNumberTable
-keep public class * extends java.lang.Exception
-keep class com.crashlytics.** { *; }
-dontwarn com.crashlytics.**
-keep class com.activeandroid.** { *; }
-keep class com.activeandroid.**.** { *; }
-keep class * extends com.activeandroid.Model
-keep class * extends com.activeandroid.serializer.TypeSerializer
-keep public class org.uoyabause.android.YabauseRunnable.** { *; }
-keep class org.uoyabause.android.Yabause.** { *; }
-keepclassmembers class **.Yabause { *; }

 # Add this global rule
 -keepattributes Signature

 # This rule will properly ProGuard all the model classes in
 # the package com.yourcompany.models. Modify to fit the structure
 # of your app.
 -keepclassmembers class org.uoyabause.android.backup.BackupItem {
      *;
 }

 -keepclassmembers class org.uoyabause.android.cheat.CheatItem {
       *;
  }

