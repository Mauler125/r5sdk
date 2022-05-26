#pragma once

// These are the base implementations of the Kore library and are required for everything
#include "ListBase.h"
#include "StringBase.h"
#include "DictionaryBase.h"
#include "ImmutableStringBase.h"

// Data\* is used in several support classes so it will always be defined
#include "Pattern.h"
#include "SecureString.h"

// System\* is always included and used for organization
#include "Environment.h"
#include "Console.h"
#include "ConsoleKey.h"
#include "ConsoleColor.h"
#include "ConsoleKeyInfo.h"
#include "Settings.h"

#ifndef KORE_ENABLE_IO
#define KORE_ENABLE_IO 1
#endif

#ifndef KORE_ENABLE_NET
#define KORE_ENABLE_NET 1
#endif

#ifndef KORE_ENABLE_THREAD
#define KORE_ENABLE_THREAD 1
#endif

#ifndef KORE_ENABLE_DIAG
#define KORE_ENABLE_DIAG 1
#endif

#ifndef KORE_ENABLE_MATH
#define KORE_ENABLE_MATH 1
#endif

#ifndef KORE_ENABLE_WIN32
#define KORE_ENABLE_WIN32 1
#endif

#ifndef KORE_ENABLE_COMP
#define KORE_ENABLE_COMP 1
#endif

#ifndef KORE_ENABLE_HASH
#define KORE_ENABLE_HASH 1
#endif

#ifndef KORE_ENABLE_FORMS
#define KORE_ENABLE_FORMS 1
#endif

#ifndef KORE_ENABLE_DRAWING
#define KORE_ENABLE_DRAWING 1
#endif

#ifndef KORE_ENABLE_ASSETS
#define KORE_ENABLE_ASSETS 1
#endif

#if KORE_ENABLE_IO
#include "Path.h"
#include "File.h"
#include "Stream.h"
#include "IOError.h"
#include "FileMode.h"
#include "FileShare.h"
#include "Directory.h"
#include "SeekOrigin.h"
#include "FileAccess.h"
#include "FileStream.h"
#include "TextReader.h"
#include "TextWriter.h"
#include "MemoryStream.h"
#include "ProcessStream.h"
#include "ProcessReader.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#endif

#if KORE_ENABLE_NET
#include "Uri.h"
#include "InternetPortType.h"
#endif

#if KORE_ENABLE_THREAD
#include "Task.h"
#include "Thread.h"
#include "ParallelTask.h"
#include "ThreadStart.h"
#endif

#if KORE_ENABLE_DIAG
#include "Process.h"
#include "ProcessInfo.h"
#endif

#if KORE_ENABLE_MATH
#include "MathHelper.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Quaternion.h"
#include "Matrix.h"
#include "Half.h"
#endif

#if KORE_ENABLE_WIN32
#include "RegistryView.h"
#include "RegistryHive.h"
#include "RegistryKey.h"
#include "RegistryValueType.h"
#include "Registry.h"
#include "Win32Error.h"
#include "clipboard/clip.h"
#endif

#if KORE_ENABLE_COMP
#include "LZ4Codec.h"
#include "LZ4Stream.h"
#include "ZLibCodec.h"
#include "ZipArchive.h"
#include "DeflateStream.h"
#include "DeflateCodec.h"
#include "LZO1XCodec.h"
#include "LZHAMCodec.h"
#include "CompressionMode.h"
#endif

#if KORE_ENABLE_HASH
#include "XXHash.h"
#include "CRC32.h"
#endif

#if KORE_ENABLE_FORMS
#include "Application.h"
#include "Form.h"
#include "Keys.h"
#include "Control.h"
#include "ButtonBase.h"
#include "Button.h"
#include "Panel.h"
#include "GroupBox.h"
#include "Label.h"
#include "MessageBox.h"
#include "ListView.h"
#include "ComboBox.h"
#include "ToolTip.h"
#include "CheckBox.h"
#include "ListViewItem.h"
#include "RadioButton.h"
#include "Message.h"
#include "FlatStyle.h"
#include "TextBox.h"
#include "TextBoxBase.h"
#include "BorderStyle.h"
#include "ProgressBar.h"
#include "DialogResult.h"
#include "OpenGLViewport.h"
#include "CreateParams.h"
#include "MouseButtons.h"
#include "AnchorStyles.h"
#include "ControlStyles.h"
#include "OpenFileDialog.h"
#include "SaveFileDialog.h"
#include "ControlStates.h"
#include "PaintEventArgs.h"
#include "MouseEventArgs.h"
#include "KeyEventArgs.h"
#include "InvalidateEventArgs.h"
#include "KeyPressEventArgs.h"
#endif

#if KORE_ENABLE_DRAWING
#include "Font.h"
#include "Icon.h"
#include "DrawingBase.h"
#include "TextRenderer.h"
#include "ContentAlignment.h"
#include "BufferedGraphics.h"
#endif

#if KORE_ENABLE_ASSETS
#include "Model.h"
#include "DDS.h"
#include "WAV.h"
#include "WavefrontOBJ.h"
#include "Texture.h"
#include "AssetRenderer.h"
#include "UnrealEngine.h"
#include "ValveSMD.h"
#include "SEAsset.h"
#include "XNALaraAscii.h"
#include "KaydaraFBX.h"
#include "AutodeskMaya.h"
#include "XNALaraBinary.h"
#include "CoDXAssetExport.h"
#endif