<div align="center">

<img src="https://i.ibb.co/8LXMML3k/icon.png" width="160" height="160" style="object-fit: contain; drop-shadow: 0px 4px 6px rgba(0,0,0,0.1);" alt="MyApkTool Pro Icon"/>

# 🛡️ MyApkTool Pro

**The Ultimate, Professional APK Reverse Engineering & Android Management Toolkit for Windows**

<p>
  <a href="https://github.com/alisakkaf/MyApkTool-Pro/releases"><img src="https://img.shields.io/github/v/release/alisakkaf/MyApkTool-Pro?style=for-the-badge&color=blue" alt="Release"></a>
  <a href="https://xdaforums.com/t/windows-tool-myapktool-pro-advanced-gui-for-apktool-signer-zipalign-adb-management-toolkit.4781628/"><img src="https://img.shields.io/badge/XDA_Forums-Official_Thread-F5A201?style=for-the-badge&logo=xda" alt="XDA Forums"></a>
  <a href="https://www.qt.io"><img src="https://img.shields.io/badge/Built_with-Qt_5.14.2-41CD52?style=for-the-badge&logo=qt" alt="Qt"></a>
  <a href="https://cppreference.com"><img src="https://img.shields.io/badge/C++-14-00599C?style=for-the-badge&logo=cplusplus" alt="C++"></a>
  <a href="https://www.microsoft.com/windows"><img src="https://img.shields.io/badge/Platform-Windows-0078D7?style=for-the-badge&logo=windows" alt="Platform"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge" alt="License"></a>
  <a href="https://mysterious-dev.com/"><img src="https://img.shields.io/badge/Developer-Ali%20Sakkaf-black?style=for-the-badge&logo=codeforces" alt="Developer"></a>
</p>

*Meticulously crafted by **Ali Sakkaf** (Active since 2016)*

<br>
<i>Empowering malware analysts, penetration testers, and Android developers with an unmatched GUI experience.</i>
<br><br>

</div>

---

## 📖 Table of Contents
<details open>
<summary>Click to expand / collapse</summary>

1. [About The Project](#about)
2. [Important Disclaimer](#disclaimer)
3. [Python Version Available](#python-version)
4. [Deep-Dive Feature Showcase](#features)
   - [1. Advanced Decompilation & Compilation Engine](#engine)
   - [2. Cryptography, Signing & Zipalign](#crypto)
   - [3. DEX Manipulation, Smali & APK Merging](#dex)
   - [4. Ultimate ADB Device Manager & WiFi Pairing](#adb)
   - [5. Professional UI/UX & Architecture](#uiux)
5. [Prerequisites & System Requirements](#prerequisites)
6. [Installation & Usage Guide](#installation)
7. [Project Architecture & Source Tree](#architecture)
8. [Embedded Tools & Open Source Licenses](#tools)
9. [License & Copyright](#license)
10. [Author & Contact](#contact)

</details>

---

<a id="about"></a>
## 🚀 About The Project

**MyApkTool Pro** is a meticulously crafted, all-in-one desktop application designed specifically for professional Android developers, malware analysts, reverse engineers, and penetration testers. 

Modifying, compiling, signing, and managing Android applications manually via the Command Line Interface (CLI) is tedious, error-prone, and time-consuming. MyApkTool Pro solves this by wrapping the world's most powerful Android CLI tools—such as `apktool`, `APKEditor`, `zipalign`, `apksigner`, `baksmali`, and `adb`—into a stunning, high-performance Graphical User Interface (GUI) powered by **C++ and the Qt Framework**.

Whether you are translating an app, removing bloatware, patching vulnerabilities, or managing a fleet of Android devices over a wireless network, MyApkTool Pro provides an uninterrupted, fluid workflow.

---

<a id="disclaimer"></a>
## ⚠️ Important Disclaimer

> [!CAUTION]
> Please read the full [DISCLAIMER.md](DISCLAIMER.md) before downloading or building this project.  
> **MyApkTool Pro** is provided strictly for **educational purposes, personal development, and authorized security auditing**. The developer (Ali Sakkaf) assumes no liability and is not responsible for any misuse, damage, or illegal activities caused by utilizing this software. Always respect copyright laws and terms of service of the applications you analyze.

---

<a id="python-version"></a>
## 🐍 Python Version Available (NEW!)

For developers and researchers who prefer **Python** over C++, I am thrilled to announce a collaboration with the brilliant **AmrKhaled**! 
We have ported **MyApkTool Pro** into a fully functional Python application retaining the exact same powerful GUI, features, and capabilities.

If you love hacking Python scripts and customizing your tools on the fly without setting up a C++ compiler, this is for you!

- **Python Repository:** [AmrKhaled-Tech/ApkTool-Pro](https://github.com/AmrKhaled-Tech/ApkTool-Pro)
- **Direct Download (Zip):** [Download Python Version Here](https://github.com/AmrKhaled-Tech/ApkTool-Pro/archive/refs/heads/main.zip)

---

<a id="features"></a>
## ✨ Deep-Dive Feature Showcase

<a id="engine"></a>
### 1. Advanced Decompilation & Compilation Engine
The core of MyApkTool Pro is its robust APK manipulation engine. It doesn't just run tools; it intelligently manages their environments.
* **Dual-Engine Support:** Switch instantly between **Apktool** and **APKEditor** depending on the complexity of the APK. Apktool is excellent for deep resource editing, while APKEditor is heavily optimized for speed and bypassing anti-decompilation tricks.
* **Deep Decoding:** Accurately decodes `resources.arsc` to readable XML files and `classes.dex` to Smali source code.
* **Modern App Compatibility:** Full support for **AAPT2** and massive APKs containing multiple DEX files (`classes2.dex` through `classes9.dex`).
* **Framework Management:** Working on system apps? Easily install custom OEM framework files (Samsung, Xiaomi, etc.) via the UI to decompile proprietary apps without errors. Includes a one-click "Clear Framework Cache" feature.
* **Smart Compilation:** Auto-detects the decompiler engine based on the project folder (e.g., checking for `apktool.yml`). Compiles with an explicit `-o` output flag to prevent naming conflicts, automatically copying the final build to a dedicated `output/` folder and highlighting it in Windows Explorer.

<a id="crypto"></a>
### 2. Cryptography, Signing & Zipalign
Security constraints in modern Android versions require precise signing schemes. We've automated the entire cryptographic process.
* **V1, V2, and V3 Signing:** Powered by `apksigner.jar`, ensuring your modified APKs install perfectly on the latest Android versions.
* **Zipalign Optimization:** Configure Zipalign to run *before* or *after* signing (crucial for V2/V3 schemes) with customizable 4-byte alignment to optimize RAM usage on Android devices.
* **Advanced Keystore Manager:** Stop typing terminal commands to generate keys. Create custom `.jks` or `.keystore` files directly from a GUI dialog with full Distinguished Name (DN) fields: Common Name (CN), Organizational Unit (OU), Organization (O), Locality (L), State (ST), and Country (C).
* **Multi-Profile System:** The app securely saves your active keystore credentials (Password, Alias, Keypass) into a `config.json` file. Switch between different developer identities instantly via a QComboBox without re-typing passwords.
* **Smart Keytool Detection:** The app intelligently hunts for `keytool.exe` by checking `JAVA_HOME`, standard Oracle paths, Amazon Corretto, and Android Studio directories.
* **Fallback Test Keys:** Includes built-in AOSP test keys (`testkey.pk8` & `testkey.x509.pem`) for quick debugging.

<a id="dex"></a>
### 3. DEX Manipulation, Smali & APK Merging
For reverse engineers who want granular control over the bytecode.
* **Direct Baksmali/Smali:** No need to decompile the whole APK. Directly disassemble a standalone `.dex` file into a Smali directory using `baksmali.jar`, edit the bytecode, and reassemble it back into a `.dex` file using `smali.jar` with a single click.
* **Merge Split APKs:** Modern Google Play apps are distributed as App Bundles (Split APKs). Use the embedded APKEditor engine to browse multiple split APKs (base, config.xxhdpi, config.en) and merge them into a single, universal APK.
* **APK Metadata Extraction:** Quickly read the internal manifest without decompiling. Extract package name, version codes, requested permissions, and main activities using `aapt dump badging`.

<a id="adb"></a>
### 4. Ultimate ADB Device Manager & WiFi Pairing
Forget the command prompt. Manage your connected devices through a clean, responsive UI.
* **Smart Device Discovery:** Automatically detects and lists connected USB and WiFi devices.
* **Next-Gen WiFi Scanner (Android 11+ Pairing):** 
  * Features a highly aggressive, silent parallel ping scanner utilizing **200 threads** to discover ADB-enabled devices on your network.
  * Uses `CREATE_NO_WINDOW` flags to ensure your screen isn't spammed with flashing CMD boxes.
  * Parses your local subnet via `ipconfig` and adds smart fallback subnets (`192.168.x.x`, `172.20.10.x`, `10.0.0.x`).
  * Full GUI dialog for Android 11+ wireless debugging pairing (IP:Port + Pairing Code).
* **Package Manager:** View all installed applications on the device. Filter by System Apps, User Apps, or All. Includes a real-time search/filter bar.
* **Pull & Backup:** Extract any installed APK (including multi-split APKs) directly from the device to your PC.
* **Install & Uninstall:** Install APKs from your PC to the device (with smart fallbacks for force-downgrade `-d` or test-only `-t` flags). Uninstall packages with one click.

<a id="uiux"></a>
### 5. Professional UI/UX & Architecture
Built for those who stare at screens for 12 hours a day.
* **Dark / Light Modes:** Fully customizable Qt Style Sheets (QSS) for a stunning, eye-friendly interface.
* **Custom Frameless Window:** A modern desktop experience with a drag-to-move custom title bar, bypassing the outdated Windows native borders.
* **Dynamic Splash Screen:** A beautiful 5-second animated loading interface featuring a progress arc and real-time status messages showing the initialization of background tools.
* **Real-Time Rich Logs:** A visual console output panel that captures `stdout` and `stderr` from all command-line tools. Features color-coded text (Success = Green, Error = Red, Command = Blue, Warning = Yellow). Includes Copy, Clear, and Toggle Visibility buttons.
* **Safe Threading:** All heavy operations (decompiling, scanning) run on separate threads (`QtConcurrent` or `QProcess` asynchronous signals). The UI never freezes, and you can abort long-running tasks using the "Cancel" button.

---

<a id="prerequisites"></a>
## ⚙️ Prerequisites & System Requirements

To ensure maximum performance and compatibility, please verify your environment before running or building the application.

### Runtime Requirements (For End Users)

> [!IMPORTANT]  
> **Java is strictly required** for the `.jar` files (Apktool, apksigner, smali) to execute properly. We highly recommend installing **Amazon Corretto**.

| Requirement | Details |
|---|---|
| **Operating System** | Windows 7 / Windows 10 / Windows 11 (32-bit or 64-bit). |
| **Java Runtime (JDK/JRE)** | **Crucial:** Java is strictly required to run `.jar` tools (Apktool, Signer). Ensure the `JAVA_HOME` environment variable is set.<br/><br/>**Official Supported Downloads:**<br/>🔗 [Java 21 (LTS)](https://www.oracle.com/java/technologies/downloads/#java21)<br/>🔗 [Java 25](https://www.oracle.com/java/technologies/downloads/#java25)<br/>🔗 [Java 9 Archive](https://www.oracle.com/java/technologies/javase/javase9-archive-downloads.html) |
| **Device Drivers** | Standard Android USB drivers (OEM specific or Google Generic) if connecting via USB. |

### Build Requirements (For Developers)

| Requirement | Version / Details |
|---|---|
| **Qt Framework** | Version **5.14.2**. (A static build is highly recommended if you plan to distribute a single `.exe` file without `.dll` dependencies). |
| **Compiler** | MinGW 7.3.0 (32-bit or 64-bit). |
| **Qt Modules** | `Core`, `Gui`, `Widgets`, `Network`, `Concurrent`. |
| **Version Control** | Git (to clone the repository). |

---

<a id="installation"></a>
## 🛠️ Installation & Usage Guide

### Option 1: Quick Start (Pre-built Portable EXE)
If you just want to use the tool without messing with C++ code:
1. Navigate to the [Releases page](https://github.com/alisakkaf/MyApkTool-Pro/releases).
2. Download the latest `MyApkTool-vX.X-win32.zip`.
3. Extract the folder anywhere on your computer (e.g., `C:\MyApkTool`). The app is 100% portable.
4. Run `MyApkTool.exe`.
5. *First Launch Magic:* The application will automatically extract all embedded tools (ADB, Apktool, AAPT, etc.) into a hidden/local `tools/` directory. 
6. Go to **Settings**, verify that your Java Path is detected, and you are ready to go!

### Option 2: Build From Source (For Developers)
To compile the Qt C++ source code yourself:

**1. Clone the Source Code**
Open CMD or PowerShell:
```bash
git clone https://github.com/alisakkaf/MyApkTool-Pro.git
cd MyApkTool-Pro
```

**2. Build with Qt Creator**
- Open `MyApkTool.pro` in Qt Creator.
- Select your MinGW compiler.
- Build and Run `(Ctrl + R)`.

---

<a id="architecture"></a>
## 🏗️ Project Architecture & Source Tree

```text
📦 MyApkTool-Pro
 ┣ 📂 ui/            # UI layout files designed in Qt Designer
 ┣ 📂 src/           # Implementation code (C++)
 ┣ 📂 include/       # Header definitions (.h)
 ┣ 📂 assets/        # Visual assets (icons, styles, splash UI)
 ┣ 📂 tools/         # Auto-extracted external binaries
 ┣ 📜 MyApkTool.pro  # Build configuration file
 ┗ 📜 README.md      # Documentation (You are here)
```

---

<a id="tools"></a>
## 🧩 Embedded Tools & Open Source Licenses

MyApkTool Pro acts as a GUI wrapper around various phenomenal open-source tools. Full credit goes to their respective authors:

* **[Apktool](https://ibotpeaches.github.io/Apktool/)** - By iBotPeaches
* **[APKEditor](https://github.com/REAndroid/APKEditor)** - By REAndroid
* **[Smali/Baksmali](https://github.com/JesusFreke/smali)** - By JesusFreke
* **[ADB & Zipalign & AAPT](https://developer.android.com/studio/command-line)** - By Google / Android Open Source Project
* **[Uber-APK-Signer](https://github.com/patrickfav/uber-apk-signer)** / apksigner - By Google & patrickfav

---

<a id="license"></a>
## ⚖️ License & Copyright

Distributed under the **MIT License**. See `LICENSE` for more information.
*You are free to use, modify, and distribute this software, as long as the original copyright notice is retained.*

---

<a id="contact"></a>
## 👨‍💻 Author & Contact

**Ali Sakkaf**
* **GitHub:** [@alisakkaf](https://github.com/alisakkaf)
* **Portfolio:** [mysterious-dev.com](https://mysterious-dev.com/)
* **XDA Forums Thread:** [MyApkTool Pro Discussion](https://xdaforums.com/t/windows-tool-myapktool-pro-advanced-gui-for-apktool-signer-zipalign-adb-management-toolkit.4781628/)

<p align="center">
  <i>If you found this tool helpful, don't forget to give this repository a ⭐️ to help it reach more developers!</i>
</p>
