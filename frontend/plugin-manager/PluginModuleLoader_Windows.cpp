/******************************************************************************
    Copyright (C) 2026 by FiniteSingularity <finitesingularityttv@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "PluginModuleLoader.hpp"
#include "PluginManager.hpp"

#include <obs.h>
#include <util/dstr.h>
#include <util/platform.h>

#include <string>
#include <string_view>
#include <vector>
#include <cctype>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

// Windows Third-Party Plugin Locations
//
// * User Path: <Environment Variable>
//     * Binary: <Environment Variable>/<Plugin>/<Plugin>.dll
//     * Data: <Environment Variable>/<Plugin>/data
//
// * Root Path: C:/ProgramData
//     * Binary: <Root Path>/obs-studio/plugins/<Plugin>/<Plugin>.dll
//     * Data: <Root Path>/obs-studio/plugins/<Plugin>/data
//
// * Portable Root Path: <OBS Binary Location>
//     * Binary: <Portable Root Path>/../../plugins/<Plugin>/<Plugin>.dll
//     * Data: <Portable Root Path>/../../plugins/<Plugin>/data
//
// * Legacy User Path: <Environment Variable> + <Environment Data Variable>
//     * Binary: <Environment Variable>/<Plugin>.dll
//     * Data: <Environment Data Variable>/<Plugin>/data
//
// * Legacy Root Path: <OBS Binary Location>
//     * Binary: <Legacy Root Path>/../../obs-plugins/<Plugin>.dll
//     * Data: <Legacy Root Path>/../../data/obs-plugins/<Plugin>
//
// * Legacy Portable Path: <OBS Binary Location>
//     * Binary: <Legacy Root Path>/../../obs-plugins/<Plugin>.dll
//     * Data: <Legacy Root Path>/../../data/obs-plugins/<Plugin>
//
// * Legacy System Path: C:/ProgramData
//     * Binary: <Legacy System Path>/obs-studio/plugins/<Plugin>/bin/<Plugin>.dll
//     * Data: <Legacy System Path>/obs-studio/plugins/<Plugin>/data
//
// * OBS Studio install (third-party plugins such as Aitum Multistream):
//     * Binary: <OBS Studio install>/obs-plugins/64bit/<Plugin>.dll
//     * Data: <OBS Studio install>/data/obs-plugins/<Plugin>
//   XStream is a separate program, so the legacy path relative to its own
//   exe does not see plugins the OBS installer put next to obs64.exe.
//   Core modules, including obs-browser, are not loaded from that install.
//   They ship inside this build under core/.
//

using State = OBS::PluginManager::State;
using ModuleType = obs_runtime_module_type;

constexpr std::string_view kPluginPathSuffix{"obs-studio/plugins/%module%"};
constexpr std::string_view kUserPluginPathSuffix{"/%module%"};
constexpr std::string_view kPortablePluginPath{"../../plugins/%module%"};

constexpr std::string_view kLegacyPortableBinaryPath{"../../obs-plugins/64bit/"};
constexpr std::string_view kLegacyPortableDataPath{"../../data/obs-plugins/%module%"};

namespace {
bool isExistingDirectory(const std::string &path)
{
	const DWORD attributes = GetFileAttributesA(path.c_str());
	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

std::string readRegistryString(HKEY root, const char *subKey, const char *valueName, REGSAM view)
{
	HKEY key = nullptr;
	if (RegOpenKeyExA(root, subKey, 0, KEY_QUERY_VALUE | view, &key) != ERROR_SUCCESS) {
		return {};
	}

	DWORD type = 0;
	DWORD size = 0;
	const char *name = valueName ? valueName : "";
	LSTATUS status = RegQueryValueExA(key, name, nullptr, &type, nullptr, &size);
	if (status != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ) || size < 2) {
		RegCloseKey(key);
		return {};
	}

	std::string value(size, '\0');
	status = RegQueryValueExA(key, name, nullptr, &type, reinterpret_cast<LPBYTE>(value.data()), &size);
	RegCloseKey(key);
	if (status != ERROR_SUCCESS) {
		return {};
	}

	const auto terminator = value.find('\0');
	if (terminator != std::string::npos) {
		value.resize(terminator);
	}

	if (type == REG_EXPAND_SZ) {
		char expanded[MAX_PATH];
		const DWORD expandedLength = ExpandEnvironmentStringsA(value.c_str(), expanded, MAX_PATH);
		if (expandedLength > 0 && expandedLength < MAX_PATH) {
			value = expanded;
		}
	}

	while (!value.empty() && (value.back() == '\\' || value.back() == '/')) {
		value.pop_back();
	}
	return value;
}

std::string canonicalDirectory(const std::string &path)
{
	char buffer[MAX_PATH];
	const DWORD length = GetFullPathNameA(path.c_str(), MAX_PATH, buffer, nullptr);
	std::string full = (length > 0 && length < MAX_PATH) ? std::string(buffer) : path;

	for (char &character : full) {
		if (character == '/') {
			character = '\\';
		} else {
			character = static_cast<char>(tolower(static_cast<unsigned char>(character)));
		}
	}

	while (full.size() > 3 && (full.back() == '\\' || full.back() == '/')) {
		full.pop_back();
	}
	return full;
}

std::string executableDirectory()
{
	wchar_t widePath[MAX_PATH];
	const DWORD length = GetModuleFileNameW(nullptr, widePath, MAX_PATH);
	if (length == 0 || length >= MAX_PATH) {
		return {};
	}

	const int bytes = WideCharToMultiByte(CP_UTF8, 0, widePath, -1, nullptr, 0, nullptr, nullptr);
	if (bytes <= 0) {
		return {};
	}

	std::string path(static_cast<size_t>(bytes), '\0');
	WideCharToMultiByte(CP_UTF8, 0, widePath, -1, path.data(), bytes, nullptr, nullptr);
	if (!path.empty() && path.back() == '\0') {
		path.pop_back();
	}

	const auto slash = path.find_last_of("\\/");
	if (slash == std::string::npos) {
		return {};
	}
	return path.substr(0, slash);
}

std::string obsStudioInstallDirectory()
{
	const char *installKeys[] = {"Software\\OBS Studio", "SOFTWARE\\OBS Studio"};
	const char *uninstallKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\OBS Studio";
	const REGSAM views[] = {KEY_WOW64_64KEY, KEY_WOW64_32KEY};
	const HKEY roots[] = {HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER};

	auto legacyDirectory = [](const std::string &installDir) {
		if (installDir.empty()) {
			return std::string();
		}
		const std::string plugins = installDir + "\\obs-plugins\\64bit";
		return isExistingDirectory(plugins) ? installDir : std::string();
	};

	for (HKEY root : roots) {
		for (REGSAM view : views) {
			for (const char *key : installKeys) {
				std::string found = legacyDirectory(readRegistryString(root, key, "", view));
				if (!found.empty()) {
					return found;
				}
				found = legacyDirectory(readRegistryString(root, key, "InstallPath", view));
				if (!found.empty()) {
					return found;
				}
			}

			std::string found =
				legacyDirectory(readRegistryString(root, uninstallKey, "InstallLocation", view));
			if (!found.empty()) {
				return found;
			}
		}
	}

	const char *programFiles[] = {"%ProgramW6432%\\obs-studio", "%ProgramFiles%\\obs-studio"};
	for (const char *pattern : programFiles) {
		char expanded[MAX_PATH];
		const DWORD expandedLength = ExpandEnvironmentStringsA(pattern, expanded, MAX_PATH);
		if (expandedLength > 0 && expandedLength < MAX_PATH) {
			std::string found = legacyDirectory(expanded);
			if (!found.empty()) {
				return found;
			}
		}
	}

	return {};
}

State loadPluginsFromPath(const std::string &pathString, ModuleType type, ModuleList &failedModules)
{
	std::string binaryPath{pathString};
	std::string dataPath{pathString};

	switch (type) {
	case OBS_MODULE_TYPE_LEGACY_PLUGIN: {
		binaryPath.append("/bin/64bit");
		dataPath.append("/data");
		break;
	}
	default: {
		dataPath.append("/data");
		break;
	}
	}

	ModuleLoadInfo info = {0};
	info.path_info.binary = binaryPath.c_str();
	info.path_info.data = dataPath.c_str();
	info.type = type;
	info.name = nullptr;

	int failedModuleCount = loadPluginsByInfo(info, failedModules);

	State result = (failedModuleCount > 0) ? State::PartialFailure : State::Success;

	return result;
}
} // namespace

namespace OBS {
State PluginManager::loadPlugins(bool usePortableMode)
{
	State userPluginState = State::Failure;
	std::string userPluginPath = getEnvironmentVariable(kPathVariable);

	if (!userPluginPath.empty()) {
		userPluginPath.append(kUserPluginPathSuffix);

		userPluginState = loadPluginsFromPath(userPluginPath, OBS_MODULE_TYPE_PLUGIN, failedModules_);
	} else {
		userPluginState = State::Success;
	}

	State libraryPluginState = State::Failure;
	std::string appLibraryPath{};

	if (!usePortableMode) {
		appLibraryPath.resize(MAX_PATH);
		int bufferSize = os_get_program_data_path(appLibraryPath.data(), appLibraryPath.size(),
							  kPluginPathSuffix.data());
		if (bufferSize > 0) {
			appLibraryPath.resize(bufferSize);
		}
	} else {
		appLibraryPath = kPortablePluginPath;
	}

	libraryPluginState = loadPluginsFromPath(appLibraryPath, OBS_MODULE_TYPE_PLUGIN, failedModules_);

	return (userPluginState && libraryPluginState) ? State::Success : State::PartialFailure;
}

State PluginManager::loadLegacyPlugins(bool usePortableMode)
{
	State userPluginState = State::Failure;

	std::string userPluginPath = getEnvironmentVariable(kLegacyBinaryPathVariable);
	std::string userDataPath = getEnvironmentVariable(kLegacyDataPathVariable);

	if (!userPluginPath.empty() && !userDataPath.empty()) {
		userDataPath.append(kUserPluginPathSuffix);

		ModuleLoadInfo info = {0};
		info.path_info.binary = userPluginPath.c_str();
		info.path_info.data = userDataPath.c_str();
		info.type = OBS_MODULE_TYPE_LEGACY_PLUGIN;
		info.name = nullptr;

		int failedModuleCount = loadPluginsByInfo(info, failedModules_);

		userPluginState = (failedModuleCount > 0) ? State::PartialFailure : State::Success;
	} else {
		userPluginState = State::Success;
	}

	State portablePluginState = State::Failure;

	{
		ModuleLoadInfo info = {0};
		info.path_info.binary = kLegacyPortableBinaryPath.data();
		info.path_info.data = kLegacyPortableDataPath.data();
		info.type = OBS_MODULE_TYPE_LEGACY_PLUGIN;
		info.name = nullptr;

		int failedModuleCount = loadPluginsByInfo(info, failedModules_);

		portablePluginState = (failedModuleCount > 0) ? State::PartialFailure : State::Success;
	}

	State obsInstallPluginState = State::Success;
	const std::string obsInstallDirectory = obsStudioInstallDirectory();
	std::string obsPluginDirectory;
	std::string obsPluginDataDirectory;
	if (!obsInstallDirectory.empty()) {
		obsPluginDirectory = obsInstallDirectory + "\\obs-plugins\\64bit";
		obsPluginDataDirectory = obsInstallDirectory + "\\data\\obs-plugins\\%module%";
		const std::string exeDirectory = executableDirectory();
		const std::string ownPluginDirectory =
			exeDirectory.empty() ? std::string()
					     : canonicalDirectory(exeDirectory + "\\..\\..\\obs-plugins\\64bit");
		if (!ownPluginDirectory.empty() && canonicalDirectory(obsPluginDirectory) == ownPluginDirectory) {
			blog(LOG_INFO, "OBS Studio plugin directory is this XStream install; not scanning it twice");
		} else {
			for (char &character : obsPluginDirectory) {
				if (character == '\\') {
					character = '/';
				}
			}
			for (char &character : obsPluginDataDirectory) {
				if (character == '\\') {
					character = '/';
				}
			}
			blog(LOG_INFO, "Loading third-party plugins installed for OBS Studio from '%s'",
			     obsPluginDirectory.c_str());

			ModuleLoadInfo info = {0};
			info.path_info.binary = obsPluginDirectory.c_str();
			info.path_info.data = obsPluginDataDirectory.c_str();
			info.type = OBS_MODULE_TYPE_LEGACY_PLUGIN;
			info.name = nullptr;

			const int failedModuleCount = loadPluginsByInfo(info, failedModules_);
			obsInstallPluginState = (failedModuleCount > 0) ? State::PartialFailure : State::Success;
		}
	}

	State libraryPluginState = State::Failure;

	if (!usePortableMode) {
		std::string appLibraryPath{};
		appLibraryPath.resize(MAX_PATH);
		int bufferSize = os_get_program_data_path(appLibraryPath.data(), appLibraryPath.size(),
							  kPluginPathSuffix.data());
		if (bufferSize > 0) {
			appLibraryPath.resize(bufferSize);
			libraryPluginState =
				loadPluginsFromPath(appLibraryPath, OBS_MODULE_TYPE_LEGACY_PLUGIN, failedModules_);
		}
	} else {
		libraryPluginState = State::Success;
	}

	return (userPluginState && portablePluginState && obsInstallPluginState && libraryPluginState)
		       ? State::Success
		       : State::PartialFailure;
}
} // namespace OBS
