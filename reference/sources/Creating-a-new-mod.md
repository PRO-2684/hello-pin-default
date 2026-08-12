Each mod is a single C++ file which is compiled into a dynamic library. After it's compiled, the mod is loaded by the Windhawk engine which calls the mod's callback functions and provides API functions that the mod can use.

The mod is loaded in the context of the processes that the mod targets. For example, if the mod targets Notepad, it will be loaded by the Windhawk engine in the context of each notepad.exe process.

In addition to the C++ code itself, the mod must provide information such as the mod id and name. Optionally, the mod may include a readme with information and settings to configure the mod.

To submit a mod to the official collection of Windhawk mods, refer to the readme in the [windhawk-mods](https://github.com/ramensoftware/windhawk-mods) repository.

## Mod metadata

Each mod starts with a metadata block which contains information such as the mod id and name. The general format is:

```
// ==WindhawkMod==
// @key value
// ==/WindhawkMod==
```

Some keys may have multiple values, and some keys can be localized.

Metadata block example:

```
// ==WindhawkMod==
// @id              new-mod
// @name            Your Awesome Mod
// @description     The best mod ever that does great things
// @version         0.1
// @author          You
// @github          https://github.com/nat
// @twitter         https://twitter.com/jack
// @homepage        https://your-personal-homepage.example.com/
// @include         mspaint.exe
// @compilerOptions -lcomdlg32
// @license         MIT
// ==/WindhawkMod==
```

### @id

```
// @id mod-id-1
```

Each mod must have a unique mod id. The mod id must only contain the following characters: 0-9, a-z, and a hyphen (-).

### @name

```
// @name The mod name
```

The mod name. Can be localized (see below).

### @description

```
// @description The mod description
```

A short description of the mod. Can be localized (see below).

### @version

```
// @version 1.0
```

The mod version number. Must increase with each newly published mod version.

### @author

```
// @author John Doe
```

The mod author name or nickname. Can be localized (see below).

### @github

```
// @github https://github.com/nat
```

The link to the GitHub profile of the mod author.

### @twitter

```
// @twitter https://twitter.com/jack
```

The link to the Twitter profile of the mod author.

### @homepage

```
// @homepage https://your-personal-homepage.example.com/
```

The link to the website of the mod author.

### @donateUrl

```
// @donateUrl https://your-personal-homepage.example.com/sponsor
```

The link to a web page where users can financially support or sponsor the mod author.

### @license

```
// @license MIT
```

The type of license under which the mod is released, specifying the terms for use, modification, and distribution.

### @include

```
// @include notepad.exe
// @include program-1.*.exe
// @include C:\programs\*.exe
// @include %SystemRoot%\explorer.exe
```

A list of executable file names/paths that the mod targets. For each process, the mod is loaded if the executable file path matches one of the `@include` entries and doesn't match any `@exclude` entry (see blow).

A wildcard can be used, where the `*` symbol matches any sequence of characters and `?` matches any single character.

Environment variables, such as `%SystemRoot%` and `%ProgramFiles%`, can be used. Note that `%ProgramFiles%` always refers to the native Program Files folder. `%ProgramFiles(X86)%` can be used to refer to the 32-bit Program Files folder (usually `C:\Program Files (x86)`).

There can be any number of `@include` entries.

### @exclude

```
// @exclude notepad.exe
// @exclude program-1.*.exe
// @exclude C:\programs\*.exe
// @exclude %SystemRoot%\explorer.exe
```

A list of executable file names/paths to be excluded from targeting. For each process, the mod is loaded if the executable file path matches one of the `@include` entries (see above) and doesn't match any `@exclude` entry.

A wildcard can be used, where the `*` symbol matches any sequence of characters and `?` matches any single character.

Environment variables, such as `%SystemRoot%` and `%ProgramFiles%`, can be used. Note that `%ProgramFiles%` always refers to the native Program Files folder. `%ProgramFiles(X86)%` can be used to refer to the 32-bit Program Files folder (usually `C:\Program Files (x86)`).

There can be any number of `@exclude` entries.

### @architecture

```
// @architecture x86
// @architecture x86-64
// @architecture amd64
// @architecture arm64
```

A list of supported architectures. Specifying no `@architecture` entries is equivalent to specifying both `x86` and `x86-64`.

Due to compatibility reasons, the names might be slightly misleading.

* **x86** - The mod is compiled as 32-bit (x86).
* **amd64** - The mod is compiled as 64-bit (x86-64).
* **arm64** - The mod is compiled as ARM64 (AArch64). Ignored on non-ARM64 devices.
* **x86-64** - On non-ARM64 devices, equivalent to **amd64**. On ARM64 devices: if the mod only targets processes from the predefined list below, equivalent to **arm64**. Otherwise, equivalent to specifying both **amd64** and **arm64**.
  * The predefined list of processes: StartMenuExperienceHost.exe, SearchHost.exe, explorer.exe, ShellExperienceHost.exe, ShellHost.exe, dwm.exe, notepad.exe, regedit.exe.

There can be any number of `@architecture` entries.

### @compilerOptions

```
// @compilerOptions -lcomctl32 -lgdi32 -luxtheme
```

Extra command line parameters that are passed to the compiler when compiling the mod.

## Mod metadata localization

Some of the metadata entries can be localized for multiple languages, for example:

```
// @name       Example mod
// @name:uk-UA Приклад мод
// @name:fr-FR Exemple de mod
```

## Readme

A mod may contain a readme block with information for the users. The general format is:

```
// ==WindhawkModReadme==
/*
content
*/
// ==/WindhawkModReadme==
```

[Markdown](https://en.wikipedia.org/wiki/Markdown) can be used to add links and formatting to the readme. Only images from the following domains can be embedded in the readme: `https://i.imgur.com`, `https://raw.githubusercontent.com`.

Readme example:

```
// ==WindhawkModReadme==
/*
# Your Awesome Mod
This is a place for useful information about your mod. Use it to describe
the mod, explain why it's useful, and add any other relevant details.
You can use [Markdown](https://en.wikipedia.org/wiki/Markdown) to add links
and **formatting** to the readme.
*/
// ==/WindhawkModReadme==
```

## Settings

A mod may define settings that the mod users will be able to configure. The settings are defined in [YAML](https://en.wikipedia.org/wiki/YAML) format as following:

```
// ==WindhawkModSettings==
/*
- SettingName: Default Value
*/
// ==/WindhawkModSettings==
```

Each setting is defined by a name and a default value. The default value is set when the mod is installed, and can be changed later by the user.

Possible setting types are: boolean, number, string, array of numbers, array of strings.

Options can be nested as can be seen in the example below.

In addition to the setting name and default value, several metadata values can be used to make it easier for users to edit the options:
* `$name`: The name of the option.
* `$description`: A short description of the option.
* `$options`: Possible values for a string option, displayed to the user in a combobox.

The metadata values can be localized for multiple languages.

Settings example:

```
// ==WindhawkModSettings==
/*
- BooleanOption: true
  $name: Example Boolean
  $description: An example boolean setting
  $description:uk-UA: Приклад логічного налаштування
- NumberOption: 1234
- StringOption: Default string value
- StringCombobox: option1
  $options:
    - option1: First Option Description
    - option2: Second Option Description
  $options:uk-UA:
    - option1: Опис першого варіанту
    - option2: Опис другого варіанту
- NestedOptions:
    - NumberNestedOption: 2345
    - StringNestedOption: Nested option text
- ArrayOfNumbers: [1, 2, 3]
- ArrayOfStrings: [a, b, c]
- ArrayOfStringComboboxes: [a, b, c]
  $options:
    - a: First Option Description
    - b: Second Option Description
    - c: Third Option Description
- ArrayOfNestedOptions:
    - - NumberNestedOptionInArray: 3456
      - StringNestedOptionInArray: Nested option in array text
*/
// ==/WindhawkModSettings==
```

## Callback functions

There are several callback functions that the mod can implement. Those callback functions are called by the Windhawk engine when specific events occur.

See also: [[Mod lifetime|Mod-lifetime]].

### `Wh_ModInit`

```
BOOL Wh_ModInit()
```

The first callback function that is called by the Windhawk engine. Called before the target process starts executing, unless the process is already running when the mod is loading. Allows the mod to initialize and to set up hooks using the Wh_SetFunctionHook API function (see below).

The mod must return `TRUE` if initialization is successful. If `FALSE` is returned, no further callbacks are called and the mod is unloaded.

### `Wh_ModAfterInit`

```
void Wh_ModAfterInit()
```

Called after `Wh_ModInit` and after the Windhawk engine completes setting up hooks.

### `Wh_ModBeforeUninit`

```
void Wh_ModBeforeUninit()
```

Called when the mod is about to be unloaded, before the Windhawk engine removes hooks.

### `Wh_ModUninit`

```
void Wh_ModUninit()
```

Called when the mod is about to be unloaded, after the Windhawk engine removes hooks.

### `Wh_ModSettingsChanged`

```
// Variant 1:
void Wh_ModSettingsChanged()
// Variant 2:
BOOL Wh_ModSettingsChanged(BOOL* bReload)
```

Called when the mod settings are changed. Allows the mod to load and apply the new settings.

Variant 2 of the callback allows to unload or reload the mod after settings are changed. If the callback returns `FALSE`, the mod will be unloaded for the target process, and will stay unloaded until settings are changed again. If the callback returns `TRUE` and `*bReload` is set to `TRUE`, the mod will be reloaded after the callback returns.

## API functions

There are several API functions that the mod can use.

### `Wh_Log`

```
#define Wh_Log(message, ...) /*...*/
```

Logs a message. If logging is enabled, the message can be viewed in
the editor log output window. The arguments are only evaluated if logging
is enabled.

* `message`: The message to be logged. It can optionally contain embedded
printf-style format specifiers that are replaced by the values specified
in subsequent additional arguments and formatted as requested.

### `Wh_GetIntValue`

```
int Wh_GetIntValue(PCWSTR valueName, int defaultValue);
```

Retrieves an integer value from the mod's local storage.

* `valueName`: The name of the value to retrieve.
* `defaultValue`: The default value to be returned as a fallback.

**Returns**: The retrieved integer value. If the value doesn't exist or in case of
an error, the provided default value is returned.

### `Wh_SetIntValue`

```
BOOL Wh_SetIntValue(PCWSTR valueName, int value);
```

Stores an integer value in the mod's local storage.

* `valueName`: The name of the value to store.
* `value`: The value to store.

**Returns**: A boolean value indicating whether the function succeeded.

### `Wh_GetStringValue`

```
size_t Wh_GetStringValue(PCWSTR valueName, PWSTR stringBuffer, size_t bufferChars);
```

Retrieves a string value from the mod's local storage.

* `valueName`: The name of the value to retrieve.
* `stringBuffer`: The buffer that will receive the text, terminated with a
null character.
* `bufferChars`: The length of `stringBuffer`, in characters. The buffer
must be large enough to include the terminating null character.

**Returns**: The number of characters copied to the buffer, not including the
terminating null character. If the value doesn't exist, if the buffer is
not large enough, or in case of an error, an empty string is returned.

### `Wh_SetStringValue`

```
BOOL Wh_SetStringValue(PCWSTR valueName, PCWSTR value);
```

Stores a string value in the mod's local storage.

* `valueName`: The name of the value to store.
* `value`: A null-terminated string containing the value to store.

**Returns**: A boolean value indicating whether the function succeeded.

### `Wh_GetBinaryValue`

```
size_t Wh_GetBinaryValue(PCWSTR valueName, void* buffer, size_t bufferSize);
```

Retrieves a binary value (raw bytes) from the mod's local storage.

* `valueName`: The name of the value to retrieve.
* `buffer`: The buffer that will receive the value.
* `bufferSize`: The length of the buffer, in bytes.

**Returns**: The number of bytes copied to the buffer. If the value doesn't exist,
if the buffer is not large enough, or in case of an error, no data is
copied and the return value is zero.

### `Wh_SetBinaryValue`

```
BOOL Wh_SetBinaryValue(PCWSTR valueName, const void* buffer, size_t bufferSize);
```

Stores a binary value (raw bytes) in the mod's local storage.

* `valueName`: The name of the value to store.
* `buffer`: An array of bytes containing the value to store.
* `bufferSize`: The size of the array of bytes.

**Returns**: A boolean value indicating whether the function succeeded.

### `Wh_DeleteValue`

```
BOOL Wh_DeleteValue(PCWSTR valueName);
```

Deletes a value from the mod's local storage.

* `valueName`: The name of the value to delete.

**Returns**: A boolean value indicating whether the function succeeded.

### `Wh_GetModStoragePath`

```
size_t Wh_GetModStoragePath(PWSTR pathBuffer, size_t bufferChars);
```

Retrieves the mod's storage directory path. The directory can be used by the mod
to store any necessary files. The directory will be removed when the mod is
removed.

* `pathBuffer`: The buffer that will receive the path, terminated with a null
  character.
* `bufferChars`: The length of `pathBuffer`, in characters. The buffer must be
  large enough to include the terminating null character.

**Returns**: The number of characters copied to the buffer, not including the
terminating null character. If the buffer is not large enough or in case of an
error, an empty string is returned.

### `Wh_GetIntSetting`

```
int Wh_GetIntSetting(PCWSTR valueName, ...);
```

Retrieves an integer value from the mod's user settings.

* `valueName`: The name of the value to retrieve. It can optionally contain
embedded printf-style format specifiers that are replaced by the values
specified in subsequent additional arguments and formatted as requested.

**Returns**: The retrieved integer value. If the value doesn't exist or in case of
an error, the return value is zero.

### `Wh_GetStringSetting`

```
PCWSTR Wh_GetStringSetting(PCWSTR valueName, ...);
```

Retrieves a string value from the mod's user settings. When no longer
needed, free the memory with `Wh_FreeStringSetting`.

* `valueName`: The name of the value to retrieve. It can optionally contain
embedded printf-style format specifiers that are replaced by the values
specified in subsequent additional arguments and formatted as requested.

**Returns**: The retrieved string value. If the value doesn't exist or in case of
an error, an empty string is returned.

### `Wh_FreeStringSetting`

```
void Wh_FreeStringSetting(PCWSTR string);
```

Frees a string returned by `Wh_GetStringSetting`.

* `string`: The string to free.

### `Wh_SetFunctionHook`

```
BOOL Wh_SetFunctionHook(void* targetFunction, void* hookFunction, void** originalFunction);
```

Registers a hook for the specified target function. Can't be called
after `Wh_ModBeforeUninit` returns. Registered hook operations can be
applied with `Wh_ApplyHookOperations`.

* `targetFunction`: A pointer to the target function, which will be
overridden by the detour function.
* `hookFunction`: A pointer to the detour function, which will override the
target function.
* `originalFunction`: A pointer to the trampoline function, which will be
used to call the original target function. Can be `NULL`.

**Returns**: A boolean value indicating whether the function succeeded.

### `Wh_RemoveFunctionHook`

```
BOOL Wh_RemoveFunctionHook(void* targetFunction);
```

Registers a hook to be removed for the specified target function.
Can't be called before `Wh_ModInit` returns or after `Wh_ModBeforeUninit`
returns. Registered hook operations can be applied with
`Wh_ApplyHookOperations`.

* `targetFunction`: A pointer to the target function, for which the hook
will be removed.

**Returns**: A boolean value indicating whether the function succeeded.

### `Wh_ApplyHookOperations`

```
BOOL Wh_ApplyHookOperations();
```

Applies hook operations registered by `Wh_SetFunctionHook` and
`Wh_RemoveFunctionHook`. Called automatically by Windhawk after
`Wh_ModInit`. Can't be called before `Wh_ModInit` returns or after
`Wh_ModBeforeUninit` returns. Note: This function is very slow, avoid
using it if possible. Ideally, all hooks should be set in `Wh_ModInit`
and this function should never be used.

**Returns**: A boolean value indicating whether the function succeeded.

### `Wh_FindFirstSymbol`

```
typedef struct tagWH_FIND_SYMBOL_OPTIONS {
    // Must be set to `sizeof(WH_FIND_SYMBOL_OPTIONS)`.
    size_t optionsSize;
    // The symbol server to query. Set to `NULL` to query the Microsoft public
    // symbol server.
    PCWSTR symbolServer;
    // Set to `TRUE` to only retrieve decorated symbols, making the enumeration
    // faster. Can be especially useful for very large modules such as Chrome or
    // Firefox.
    BOOL noUndecoratedSymbols;
} WH_FIND_SYMBOL_OPTIONS;

typedef struct tagWH_FIND_SYMBOL {
    void* address;
    PCWSTR symbol;
    PCWSTR symbolDecorated;
} WH_FIND_SYMBOL;

HANDLE Wh_FindFirstSymbol(HMODULE hModule, const WH_FIND_SYMBOL_OPTIONS* options, WH_FIND_SYMBOL* findData);
```

Returns information about the first symbol for the specified module
handle.

* `hModule`: A handle to the loaded module whose information is being
requested. If this parameter is `NULL`, the module of the current process
(.exe file) is used.
* `options`: Can be used to customize the symbol enumeration. Pass `NULL`
to use the default options.
* `findData`: A pointer to a structure to receive the symbol information.

**Returns**: A search handle used in a subsequent call to `Wh_FindNextSymbol` or
`Wh_FindCloseSymbol`. If no symbols are found or in case of an error, the
return value is `NULL`.

### `Wh_FindNextSymbol`

```
typedef struct tagWH_FIND_SYMBOL {
    void* address;
    PCWSTR symbol;
    PCWSTR symbolDecorated;
} WH_FIND_SYMBOL;

BOOL Wh_FindNextSymbol(HANDLE symSearch, WH_FIND_SYMBOL* findData);
```

Returns information about the next symbol for the specified search
handle, continuing an enumeration from a previous call to
`Wh_FindFirstSymbol`.

* `symSearch`: A search handle returned by a previous call to
`Wh_FindFirstSymbol`.
* `findData`: A pointer to a structure to receive the symbol information.

**Returns**: A boolean value indicating whether symbol information was retrieved.
If no more symbols are found or in case of an error, the return value is
`FALSE`.

### `Wh_FindCloseSymbol`

```
void Wh_FindCloseSymbol(HANDLE symSearch);
```

Closes a file search handle opened by `Wh_FindFirstSymbol`.

* `symSearch`: The search handle. If symSearch is `NULL`, the function does
nothing.

### `Wh_Disasm`

```
typedef struct tagWH_DISASM_RESULT {
    // The length of the decoded instruction.
    size_t length;
    // The textual, human-readable representation of the instruction.
    char text[96];
} WH_DISASM_RESULT;

BOOL Wh_Disasm(void* address, WH_DISASM_RESULT* result);
```

Disassembles an instruction and formats it to human-readable text.

* `address`: The address of the instruction to disassemble.
* `result`: A pointer to a structure to receive the disassembly
information.

**Returns**: A boolean value indicating whether the function succeeded.

### `Wh_GetUrlContent`

```
typedef struct tagWH_GET_URL_CONTENT_OPTIONS {
    // Must be set to `sizeof(WH_GET_URL_CONTENT_OPTIONS)`.
    size_t optionsSize;
    // The path to the file to which the content will be written. If set, the
    // data will be written to the file and the `data` field of the returned
    // struct will be `NULL`. If this field is `NULL`, the content will be
    // returned in the `data` field.
    PCWSTR targetFilePath;
} WH_GET_URL_CONTENT_OPTIONS;

typedef struct tagWH_URL_CONTENT {
    const char* data;
    size_t length;
    int statusCode;
} WH_URL_CONTENT;

const WH_URL_CONTENT* Wh_GetUrlContent(
    PCWSTR url,
    const WH_GET_URL_CONTENT_OPTIONS* options);
```

Retrieves the content of a URL. When no longer needed, call
`Wh_FreeUrlContent` to free the content.

* `url`: The URL to retrieve.
* `options`: The options for the URL content retrieval. Pass `NULL` to use the
  default options.

**Returns**: The retrieved content. In case of an error, `NULL` is returned.

### `Wh_FreeUrlContent`

```
void Wh_FreeUrlContent(const WH_URL_CONTENT* content);
```

Frees the content of a URL returned by `Wh_GetUrlContent`.

* `content`: The content to free. If `NULL`, the function does nothing.

## Constants

There are several defined constants that can be used in the code.

### `WH_MOD_ID`

The mod id. Example: `L"my-mod"`

### `WH_MOD_VERSION`

The mod version. Example: `L"1.0"`

## Compilation

As of version 1.7, Windhawk uses Clang 20 ([mingw-w64 toolchain](https://github.com/mstorsjo/llvm-mingw)) and compiles the mods in C++23 mode. The full command line parameters can be seen in editing mode by clicking Ctrl+P and selecting `compile_flags.txt`.