/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2015 - ROLI Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

class AndroidStudioProjectExporter  : public ProjectExporter
{
public:
    //==============================================================================
    bool isXcode() const override                { return false; }
    bool isVisualStudio() const override         { return false; }
    bool isCodeBlocks() const override           { return false; }
    bool isMakefile() const override             { return false; }

    bool isAndroid() const override              { return true; }
    bool isWindows() const override              { return false; }
    bool isLinux() const override                { return false; }
    bool isOSX() const override                  { return false; }
    bool isiOS() const override                  { return false; }

    bool supportsVST() const override            { return false; }
    bool supportsVST3() const override           { return false; }
    bool supportsAAX() const override            { return false; }
    bool supportsRTAS() const override           { return false; }
    bool supportsAU()   const override           { return false; }
    bool supportsAUv3() const override           { return false; }
    bool supportsStandalone() const override     { return false;  }

    bool usesMMFiles() const override                       { return false; }
    bool canCopeWithDuplicateFiles() override               { return false; }
    bool supportsUserDefinedConfigurations() const override { return false; }

    bool isAndroidStudio() const override                   { return true; }

    //==============================================================================
    void addPlatformSpecificSettingsForProjectType (const ProjectType&) override
    {
        // no-op.
    }

    //==============================================================================
    void createExporterProperties (PropertyListBuilder& props) override
    {
        createBaseExporterProperties (props);
        createToolchainExporterProperties (props);
        createManifestExporterProperties (props);
        createLibraryModuleExporterProperties (props);
        createCodeSigningExporterProperties (props);
        createOtherExporterProperties (props);
    }


    static const char* getName()                         { return "Android"; }
    static const char* getValueTreeTypeName()            { return "ANDROIDSTUDIO"; }

    static AndroidStudioProjectExporter* createForSettings (Project& project, const ValueTree& settings)
    {
        if (settings.hasType (getValueTreeTypeName()))
            return new AndroidStudioProjectExporter (project, settings);

        return nullptr;
    }

    //==============================================================================
    CachedValue<String> androidScreenOrientation, androidActivityClass, androidActivitySubClassName,
                        androidVersionCode, androidMinimumSDK, androidTheme,
                        androidSharedLibraries, androidStaticLibraries;

    CachedValue<bool>   androidInternetNeeded, androidMicNeeded, androidBluetoothNeeded;
    CachedValue<String> androidOtherPermissions;

    CachedValue<String> androidKeyStore, androidKeyStorePass, androidKeyAlias, androidKeyAliasPass;

    CachedValue<String> gradleVersion, androidPluginVersion, gradleToolchain, buildToolsVersion;

    //==============================================================================
    AndroidStudioProjectExporter (Project& p, const ValueTree& t)
        : ProjectExporter (p, t),
          androidScreenOrientation (settings, Ids::androidScreenOrientation, nullptr, "unspecified"),
          androidActivityClass (settings, Ids::androidActivityClass, nullptr, createDefaultClassName()),
          androidActivitySubClassName (settings, Ids::androidActivitySubClassName, nullptr),
          androidVersionCode (settings, Ids::androidVersionCode, nullptr, "1"),
          androidMinimumSDK (settings, Ids::androidMinimumSDK, nullptr, "23"),
          androidTheme (settings, Ids::androidTheme, nullptr),
          androidSharedLibraries (settings, Ids::androidSharedLibraries, nullptr, ""),
          androidStaticLibraries (settings, Ids::androidStaticLibraries, nullptr, ""),
          androidInternetNeeded (settings, Ids::androidInternetNeeded, nullptr, true),
          androidMicNeeded (settings, Ids::microphonePermissionNeeded, nullptr, false),
          androidBluetoothNeeded (settings, Ids::androidBluetoothNeeded, nullptr, true),
          androidOtherPermissions (settings, Ids::androidOtherPermissions, nullptr),
          androidKeyStore (settings, Ids::androidKeyStore, nullptr, "${user.home}/.android/debug.keystore"),
          androidKeyStorePass (settings, Ids::androidKeyStorePass, nullptr, "android"),
          androidKeyAlias (settings, Ids::androidKeyAlias, nullptr, "androiddebugkey"),
          androidKeyAliasPass (settings, Ids::androidKeyAliasPass, nullptr, "android"),
          gradleVersion (settings, Ids::gradleVersion, nullptr, "2.14.1"),
          androidPluginVersion (settings, Ids::androidPluginVersion, nullptr, "2.2.2"),
          gradleToolchain (settings, Ids::gradleToolchain, nullptr, "clang"),
          buildToolsVersion (settings, Ids::buildToolsVersion, nullptr, "23.0.2"),
          androidStudioExecutable (findAndroidStudioExecutable())
    {
        initialiseDependencyPathValues();
        name = getName();

        if (getTargetLocationString().isEmpty())
            getTargetLocationValue() = getDefaultBuildsRootFolder() + "Android";
    }

    //==============================================================================
    void createToolchainExporterProperties (PropertyListBuilder& props)
    {
        props.add (new TextWithDefaultPropertyComponent<String> (gradleVersion, "gradle version", 32),
                   "The version of gradle that is used to build this app (2.14.1 is fine for JUCE)");

        props.add (new TextWithDefaultPropertyComponent<String> (androidPluginVersion, "android plug-in version", 32),
                   "The version of the android build plugin for gradle that is used to build this app");

        static const char* toolchains[] = { "clang", "gcc", nullptr };

        props.add (new ChoicePropertyComponent (gradleToolchain.getPropertyAsValue(), "NDK Toolchain", StringArray (toolchains), Array<var> (toolchains)),
                   "The toolchain that gradle should invoke for NDK compilation (variable model.android.ndk.tooclhain in app/build.gradle)");

        props.add (new TextWithDefaultPropertyComponent<String> (buildToolsVersion, "Android build tools version", 32),
                   "The Android build tools version that should use to build this app");
    }

    void createLibraryModuleExporterProperties (PropertyListBuilder& props)
    {
        props.add (new TextPropertyComponent (androidStaticLibraries.getPropertyAsValue(), "Import static library modules", 8192, true),
                   "Comma or whitespace delimited list of static libraries (.a) defined in NDK_MODULE_PATH.");

        props.add (new TextPropertyComponent (androidSharedLibraries.getPropertyAsValue(), "Import shared library modules", 8192, true),
                   "Comma or whitespace delimited list of shared libraries (.so) defined in NDK_MODULE_PATH.");
    }

    //==============================================================================
    bool canLaunchProject() override
    {
        return androidStudioExecutable.exists();
    }

    bool launchProject() override
    {
        if (! androidStudioExecutable.exists())
        {
            jassertfalse;
            return false;
        }

        const File targetFolder (getTargetFolder());

        // we have to surround the path with extra quotes, otherwise Android Studio
        // will choke if there are any space characters in the path.
        return androidStudioExecutable.startAsProcess ("\"" + targetFolder.getFullPathName() + "\"");
    }

    //==============================================================================
    void create (const OwnedArray<LibraryModule>& modules) const override
    {
        const File targetFolder (getTargetFolder());

        removeOldFiles (targetFolder);

        {
            const String package (getActivityClassPackage());
            const String path (package.replaceCharacter ('.', File::separator));
            const File javaTarget (targetFolder.getChildFile ("app/src/main/java").getChildFile (path));

            copyActivityJavaFiles (modules, javaTarget, package);
        }

        writeFile (targetFolder, "settings.gradle",  getSettingsGradleFileContent());
        writeFile (targetFolder, "build.gradle",     getProjectBuildGradleFileContent());
        writeFile (targetFolder, "app/build.gradle", getAppBuildGradleFileContent());
        writeFile (targetFolder, "local.properties", getLocalPropertiesFileContent());
        writeFile (targetFolder, "gradle/wrapper/gradle-wrapper.properties", getGradleWrapperPropertiesFileContent());

        writeBinaryFile (targetFolder, "gradle/wrapper/LICENSE-for-gradlewrapper.txt", BinaryData::LICENSE, BinaryData::LICENSESize);
        writeBinaryFile (targetFolder, "gradle/wrapper/gradle-wrapper.jar", BinaryData::gradlewrapper_jar, BinaryData::gradlewrapper_jarSize);
        writeBinaryFile (targetFolder, "gradlew",                           BinaryData::gradlew,           BinaryData::gradlewSize);
        writeBinaryFile (targetFolder, "gradlew.bat",                       BinaryData::gradlew_bat,       BinaryData::gradlew_batSize);

        targetFolder.getChildFile ("gradlew").setExecutePermission (true);

        writeAndroidManifest (targetFolder);
        writeStringsXML      (targetFolder);
        writeAppIcons        (targetFolder);

        const File jniFolder (targetFolder.getChildFile ("app"));
        writeApplicationMk (jniFolder.getChildFile ("Application.mk"));
        writeAndroidMk (jniFolder.getChildFile ("Android.mk"));
    }

    void removeOldFiles (const File& targetFolder) const
    {
        targetFolder.getChildFile ("app/src").deleteRecursively();
        targetFolder.getChildFile ("app/build").deleteRecursively();
        targetFolder.getChildFile ("app/build.gradle").deleteFile();
        targetFolder.getChildFile ("gradle").deleteRecursively();
        targetFolder.getChildFile ("local.properties").deleteFile();
        targetFolder.getChildFile ("settings.gradle").deleteFile();
    }

    void writeFile (const File& gradleProjectFolder, const String& filePath, const String& fileContent) const
    {
        MemoryOutputStream outStream;
        outStream << fileContent;
        overwriteFileIfDifferentOrThrow (gradleProjectFolder.getChildFile (filePath), outStream);
    }

    void writeBinaryFile (const File& gradleProjectFolder, const String& filePath, const char* binaryData, const int binarySize) const
    {
        MemoryOutputStream outStream;
        outStream.write (binaryData, static_cast<size_t> (binarySize));
        overwriteFileIfDifferentOrThrow (gradleProjectFolder.getChildFile (filePath), outStream);
    }

    //==============================================================================
    static File findAndroidStudioExecutable()
    {
       #if JUCE_WINDOWS
        const File defaultInstallation ("C:\\Program Files\\Android\\Android Studio\\bin");

        if (defaultInstallation.exists())
        {
            {
                const File studio64 = defaultInstallation.getChildFile ("studio64.exe");

                if (studio64.existsAsFile())
                    return studio64;
            }

            {
                const File studio = defaultInstallation.getChildFile ("studio.exe");

                if (studio.existsAsFile())
                    return studio;
            }
        }
      #elif JUCE_MAC
       const File defaultInstallation ("/Applications/Android Studio.app");

       if (defaultInstallation.exists())
           return defaultInstallation;
      #endif

        return File();
    }

protected:
    //==============================================================================
    class AndroidStudioBuildConfiguration  : public BuildConfiguration
    {
    public:
        AndroidStudioBuildConfiguration (Project& p, const ValueTree& settings, const ProjectExporter& e)
            : BuildConfiguration (p, settings, e)
        {
            if (getArchitectures().isEmpty())
            {
                if (isDebug())
                    getArchitecturesValue() = "armeabi x86";
                else
                    getArchitecturesValue() = "armeabi armeabi-v7a x86";
            }
        }

        Value getArchitecturesValue()           { return getValue (Ids::androidArchitectures); }
        String getArchitectures() const         { return config [Ids::androidArchitectures]; }

        var getDefaultOptimisationLevel() const override    { return var ((int) (isDebug() ? gccO0 : gccO3)); }

        void createConfigProperties (PropertyListBuilder& props) override
        {
            addGCCOptimisationProperty (props);

            props.add (new TextPropertyComponent (getArchitecturesValue(), "Architectures", 256, false),
                       "A list of the ARM architectures to build (for a fat binary).");
        }
    };

    BuildConfiguration::Ptr createBuildConfig (const ValueTree& v) const override
    {
        return new AndroidStudioBuildConfiguration (project, v, *this);
    }

private:
    static void createSymlinkAndParentFolders (const File& originalFile, const File& linkFile)
    {
        {
            const File linkFileParentDirectory (linkFile.getParentDirectory());

            // this will recursively creative the parent directories for the file.
            // without this, the symlink would fail because it doesn't automatically create
            // the folders if they don't exist
            if (! linkFileParentDirectory.createDirectory())
                throw SaveError (String ("Could not create directory ") + linkFileParentDirectory.getFullPathName());
        }

        if (! originalFile.createSymbolicLink (linkFile, true))
            throw SaveError (String ("Failed to create symlink from ")
                              + linkFile.getFullPathName() + " to "
                              + originalFile.getFullPathName() + "!");
    }

    void makeSymlinksForGroup (const Project::Item& group, const File& targetFolder) const
    {
        if (! group.isGroup())
        {
            throw SaveError ("makeSymlinksForGroup was called with something other than a group!");
        }

        for (int i = 0; i < group.getNumChildren(); ++i)
        {
            const Project::Item& projectItem = group.getChild (i);

            if (projectItem.isGroup())
            {
                makeSymlinksForGroup (projectItem, targetFolder.getChildFile (projectItem.getName()));
            }
            else if (projectItem.shouldBeAddedToTargetProject()) // must be a file then
            {
                const File originalFile (projectItem.getFile());
                const File targetFile (targetFolder.getChildFile (originalFile.getFileName()));

                createSymlinkAndParentFolders (originalFile, targetFile);
            }
        }
    }

    void createSourceSymlinks (const File& folder) const
    {
        const File targetFolder (folder.getChildFile ("app/src/main/jni"));

        // here we make symlinks to only to files included in the groups inside the project
        // this is because Android Studio does not have a concept of groups and just uses
        // the file system layout to determine what's to be compiled
        {
            const Array<Project::Item>& groups = getAllGroups();

            for (int i = 0; i < groups.size(); ++i)
            {
                const Project::Item projectItem (groups.getReference (i));
                const String projectItemName (projectItem.getName());

                if (projectItem.isGroup())
                    makeSymlinksForGroup (projectItem, projectItemName == "Juce Modules" ? targetFolder.getChildFile ("JuceModules") : targetFolder);
            }
        }
    }

    void writeAppIcons (const File& folder) const
    {
        writeIcons (folder.getChildFile ("app/src/main/res/"));
    }

    static String sanitisePath (String path)
    {
        return expandHomeFolderToken (path).replace ("\\", "\\\\");
    }

    static String expandHomeFolderToken (const String& path)
    {
        String homeFolder = File::getSpecialLocation (File::userHomeDirectory).getFullPathName();

        return path.replace ("${user.home}", homeFolder)
                   .replace ("~", homeFolder);
    }

    //==============================================================================
    struct ShouldFileBeCompiledPredicate
    {
        bool operator() (const Project::Item& projectItem) const  { return projectItem.shouldBeCompiled(); }
    };

    //==============================================================================
    void writeApplicationMk (const File& file) const
    {
        const String toolchain = gradleToolchain.get();
        const bool isClang = (toolchain == "clang");

        MemoryOutputStream mo;

        mo << "# Automatically generated makefile, created by the Projucer" << newLine
        << "# Don't edit this file! Your changes will be overwritten when you re-save the Projucer project!" << newLine
        << newLine
        << "APP_STL := " << (isClang ? "c++_static" :  "gnustl_static") << newLine
        << "APP_CPPFLAGS += " << getCppFlags() << newLine
        << "APP_PLATFORM := " << getAppPlatform() << newLine
        << "NDK_TOOLCHAIN_VERSION := " << toolchain << newLine
        << newLine
        << "ifeq ($(NDK_DEBUG),1)" << newLine
        << "    APP_ABI := " << getABIs<AndroidStudioBuildConfiguration> (true) << newLine
        << "else" << newLine
        << "    APP_ABI := " << getABIs<AndroidStudioBuildConfiguration> (false) << newLine
        << "endif" << newLine;

        overwriteFileIfDifferentOrThrow (file, mo);
    }

    void writeAndroidMk (const File& file) const
    {
        Array<RelativePath> files;

        for (int i = 0; i < getAllGroups().size(); ++i)
            findAllProjectItemsWithPredicate (getAllGroups().getReference(i), files, ShouldFileBeCompiledPredicate());

        MemoryOutputStream mo;
        writeAndroidMk (mo, files);

        overwriteFileIfDifferentOrThrow (file, mo);
    }

    void writeAndroidMkVariableList (OutputStream& out, const String& variableName, const String& settingsValue) const
    {
        const StringArray separatedItems (getCommaOrWhitespaceSeparatedItems (settingsValue));

        if (separatedItems.size() > 0)
            out << newLine << variableName << " := " << separatedItems.joinIntoString (" ") << newLine;
    }

    void writeAndroidMk (OutputStream& out, const Array<RelativePath>& files) const
    {
        out << "# Automatically generated makefile, created by the Projucer" << newLine
        << "# Don't edit this file! Your changes will be overwritten when you re-save the Projucer project!" << newLine
        << newLine
        << "LOCAL_PATH := $(call my-dir)" << newLine
        << newLine
        << "include $(CLEAR_VARS)" << newLine
        << newLine
        << "ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)" << newLine
        << "    LOCAL_ARM_MODE := arm" << newLine
        << "endif" << newLine
        << newLine
        << "LOCAL_MODULE := juce_jni" << newLine
        << "LOCAL_SRC_FILES := \\" << newLine;

        for (int i = 0; i < files.size(); ++i)
            out << "  " << (files.getReference(i).isAbsolute() ? "" : "../")
            << escapeSpaces (files.getReference(i).toUnixStyle()) << "\\" << newLine;

        writeAndroidMkVariableList (out, "LOCAL_STATIC_LIBRARIES", androidStaticLibraries);
        writeAndroidMkVariableList (out, "LOCAL_SHARED_LIBRARIES", androidSharedLibraries);

        out << newLine
        << "ifeq ($(NDK_DEBUG),1)" << newLine;
        writeConfigSettings (out, true);
        out << "else" << newLine;
        writeConfigSettings (out, false);
        out << "endif" << newLine
        << newLine
        << "include $(BUILD_SHARED_LIBRARY)" << newLine;

        StringArray importModules (getCommaOrWhitespaceSeparatedItems (androidStaticLibraries));
        importModules.addArray (getCommaOrWhitespaceSeparatedItems (androidSharedLibraries));

        for (int i = 0; i < importModules.size(); ++i)
            out << "$(call import-module," << importModules[i] << ")" << newLine;
    }

    void writeConfigSettings (OutputStream& out, bool forDebug) const
    {
        for (ConstConfigIterator config (*this); config.next();)
        {
            if (config->isDebug() == forDebug)
            {
                const AndroidStudioBuildConfiguration& androidConfig = dynamic_cast<const AndroidStudioBuildConfiguration&> (*config);

                String cppFlags;
                cppFlags << createCPPFlags (androidConfig)
                << (" " + replacePreprocessorTokens (androidConfig, getExtraCompilerFlagsString()).trim()).trimEnd()
                << newLine
                << getLDLIBS (androidConfig).trimEnd()
                << newLine;

                out << "  LOCAL_CPPFLAGS += " << cppFlags;
                out << "  LOCAL_CFLAGS += " << cppFlags;
                break;
            }
        }
    }

    String getLDLIBS (const AndroidStudioBuildConfiguration& config) const
    {
        const String toolchain = gradleToolchain.get();
        const bool isClang = (toolchain == "clang");

        return "  LOCAL_LDLIBS :=" + config.getGCCLibraryPathFlags()
        + " -llog -lGLESv2 -landroid -lEGL"
        + (isClang ? " -latomic" : "")
        + getExternalLibraryFlags (config)
        + " " + replacePreprocessorTokens (config, getExtraLinkerFlagsString());
    }

    String createIncludePathFlags (const AndroidStudioBuildConfiguration& config) const
    {
        String flags;
        StringArray searchPaths (extraSearchPaths);
        searchPaths.addArray (config.getHeaderSearchPaths());

        searchPaths = getCleanedStringArray (searchPaths);

        for (int i = 0; i < searchPaths.size(); ++i)
        {
            RelativePath searchPath =
                RelativePath (searchPaths[i], RelativePath::buildTargetFolder)
                    .rebased (getTargetFolder(), getTargetFolder().getChildFile ("app"), RelativePath::buildTargetFolder);

            flags << " -I " << FileHelpers::unixStylePath (replacePreprocessorTokens (config, searchPath.toUnixStyle())).quoted();
        }

        return flags;
    }

    String createCPPFlags (const AndroidStudioBuildConfiguration& config) const
    {
        StringPairArray defines;
        defines.set ("JUCE_ANDROID", "1");
        defines.set ("JUCE_ANDROID_API_VERSION", androidMinimumSDK.get());
        defines.set ("JUCE_ANDROID_ACTIVITY_CLASSNAME", getJNIActivityClassName().replaceCharacter ('/', '_'));
        defines.set ("JUCE_ANDROID_ACTIVITY_CLASSPATH", "\\\"" + getJNIActivityClassName() + "\\\"");

        String flags ("-fsigned-char -fexceptions -frtti");

        if (config.isDebug())
        {
            flags << " -g";
            defines.set ("DEBUG", "1");
            defines.set ("_DEBUG", "1");
        }
        else
        {
            defines.set ("NDEBUG", "1");
        }

        flags << createIncludePathFlags (config)
        << " -O" << config.getGCCOptimisationFlag();

        flags << " -std=gnu++11";

        defines = mergePreprocessorDefs (defines, getAllPreprocessorDefs (config));
        return flags + createGCCPreprocessorFlags (defines);
    }

    //==============================================================================
    String getCppFlags() const
    {
        const String toolchain = gradleToolchain.get();
        String flags ("-fsigned-char -fexceptions -frtti");

        if (! toolchain.startsWithIgnoreCase ("clang"))
            flags << " -Wno-psabi";

        return flags;
    }

    String getAppPlatform() const
    {
        int ndkVersion = androidMinimumSDK.get().getIntValue();
        if (ndkVersion == 9)
            ndkVersion = 10; // (doesn't seem to be a version '9')

        return "android-" + String (ndkVersion);
    }


    //==============================================================================
    struct GradleElement
    {
        virtual ~GradleElement() {}
        String toString() const    { return toStringIndented (0); }
        virtual String toStringIndented (int indentLevel) const = 0;

        static String indent (int indentLevel)    { return String::repeatedString ("    ", indentLevel); }
    };

    //==============================================================================
    struct GradleStatement : public GradleElement
    {
        GradleStatement (const String& s)  : statement (s) {}
        String toStringIndented (int indentLevel) const override    { return indent (indentLevel) + statement; }

        String statement;
    };

    //==============================================================================
    struct GradleCppFlag  : public GradleStatement
    {
        GradleCppFlag (const String& flag)
            : GradleStatement ("cppFlags.add(" + flag.quoted() + ")") {}
    };

    struct GradlePreprocessorDefine  : public GradleStatement
    {
        GradlePreprocessorDefine (const String& define, const String& value)
            : GradleStatement ("cppFlags.add(\"-D" + define + "=" + value + "\")") {}
    };

    struct GradleHeaderIncludePath  : public GradleStatement
    {
        GradleHeaderIncludePath (const String& path)
            : GradleStatement ("cppFlags.add(\"-I${project.rootDir}/" + sanitisePath (path) + "\".toString())") {}
    };

    struct GradleLibrarySearchPath  : public GradleStatement
    {
        GradleLibrarySearchPath (const String& path)
            : GradleStatement ("cppFlags.add(\"-L" + sanitisePath (path) + "\".toString())") {}
    };

    struct GradleLinkerFlag  : public GradleStatement
    {
        GradleLinkerFlag (const String& flag)
            : GradleStatement ("ldFlags.add(" + flag.quoted() + "\")") {}
    };

    struct GradleLinkLibrary  : public GradleStatement
    {
        GradleLinkLibrary (const String& lib)
            : GradleStatement ("ldLibs.add(" + lib.quoted() + ")") {}
    };

    //==============================================================================
    struct GradleValue : public GradleElement
    {
        template <typename ValueType>
        GradleValue (const String& k, const ValueType& v)
            : key (k), value (v) {}

        GradleValue (const String& k, bool boolValue)
            : key (k), value (boolValue ? "true" : "false") {}

        String toStringIndented (int indentLevel) const override
        {
            return indent (indentLevel) + key + " = " + value;
        }

    protected:
        String key, value;
    };

    struct GradleString : public GradleValue
    {
        GradleString (const String& k, const String& str)
            : GradleValue (k, str.quoted())
        {
            if (str.containsAnyOf ("${\"\'"))
                value += ".toString()";
        }
    };

    struct GradleFilePath : public GradleValue
    {
        GradleFilePath (const String& k, const String& path)
            : GradleValue (k, "new File(\"" + sanitisePath (path) + "\")") {}
    };

    //==============================================================================
    struct GradleObject  : public GradleElement
    {
        GradleObject (const String& nm) : name (nm) {}

       #if JUCE_COMPILER_SUPPORTS_VARIADIC_TEMPLATES
        template <typename GradleType, typename... Args>
        void add (Args... args)
        {
            children.add (new GradleType (args...));
            // Note: can't use std::forward because it doesn't compile for OS X 10.8
        }
       #else // Remove this workaround once we drop VS2012 support!
        template <typename GradleType, typename Arg1>
        void add (Arg1 arg1)
        {
            children.add (new GradleType (arg1));
        }

        template <typename GradleType, typename Arg1, typename Arg2>
        void add (Arg1 arg1, Arg2 arg2)
        {
            children.add (new GradleType (arg1, arg2));
        }
       #endif

        void addChildObject (GradleObject* objectToAdd) noexcept
        {
            children.add (objectToAdd);
        }

        String toStringIndented (int indentLevel) const override
        {
            String result;
            result << indent (indentLevel) << name << " {" << newLine;

            for (const auto& child : children)
                result << child->toStringIndented (indentLevel + 1) << newLine;

            result << indent (indentLevel) << "}";

            if (indentLevel == 0)
                result << newLine;

            return result;
        }

    private:
        String name;
        OwnedArray<GradleElement> children;
    };

    //==============================================================================
    String getSettingsGradleFileContent() const
    {
        return "include ':app'";
    }

    String getProjectBuildGradleFileContent() const
    {
        String projectBuildGradle;
        projectBuildGradle << getGradleBuildScript();
        projectBuildGradle << getGradleAllProjects();

        return projectBuildGradle;
    }

    //==============================================================================
    String getGradleBuildScript() const
    {
        GradleObject buildScript ("buildscript");

        buildScript.addChildObject (getGradleRepositories());
        buildScript.addChildObject (getGradleDependencies());

        return buildScript.toString();
    }

    GradleObject* getGradleRepositories() const
    {
        auto repositories = new GradleObject ("repositories");
        repositories->add<GradleStatement> ("jcenter()");
        return repositories;
    }

    GradleObject* getGradleDependencies() const
    {
        auto dependencies = new GradleObject ("dependencies");

        dependencies->add<GradleStatement> ("classpath 'com.android.tools.build:gradle:"
                                            + androidPluginVersion.get() + "'");
        return dependencies;
    }

    String getGradleAllProjects() const
    {
        GradleObject allProjects ("allprojects");
        allProjects.addChildObject (getGradleRepositories());
        return allProjects.toString();
    }

    //==============================================================================
    String getAppBuildGradleFileContent() const
    {
        String appBuildGradle;

        appBuildGradle << "apply plugin: 'com.android.application'" << newLine;
        appBuildGradle << getAndroidObject();
        //        appBuildGradle << getAppDependencies();

        return appBuildGradle;
    }

    String getAppDependencies() const
    {
        GradleObject dependencies ("dependencies");
        dependencies.add<GradleStatement> ("compile \"com.android.support:support-v4:+\"");
        return dependencies.toString();
    }

    //==============================================================================
    String getAndroidObject() const
    {
        GradleObject android ("android");

        android.add<GradleValue> ("compileSdkVersion", androidMinimumSDK.get().getIntValue());
        android.add<GradleString> ("buildToolsVersion", buildToolsVersion.get());
        android.addChildObject (getExternalNdkBuildObject());
        android.addChildObject (getAndroidDefaultConfig());

        return android.toString();
    }

    GradleObject* getAndroidDefaultConfig() const
    {
        const String bundleIdentifier  = project.getBundleIdentifier().toString().toLowerCase();
        const int minSdkVersion = androidMinimumSDK.get().getIntValue();

        auto defaultConfig = new GradleObject ("defaultConfig.with");

        defaultConfig->add<GradleString> ("applicationId",    bundleIdentifier);
        defaultConfig->add<GradleValue>  ("minSdkVersion",    minSdkVersion);
        defaultConfig->add<GradleValue>  ("targetSdkVersion", minSdkVersion);

        return defaultConfig;
    }

    GradleObject* getExternalNdkBuildObject() const
    {
        auto externalNdk = new GradleObject ("externalNativeBuild");
        auto ndkBuild    = new GradleObject ("ndkBuild");

        ndkBuild->add<GradleString> ("path", "Android.mk");
        externalNdk->addChildObject (ndkBuild);

        return externalNdk;
    }

#if 0
    GradleObject* getAndroidNdkSettings() const
    {
        const String toolchain = gradleToolchain.get();
        const bool isClang = (toolchain == "clang");

        auto ndkSettings = new GradleObject ("android.ndk");

        ndkSettings->add<GradleString> ("moduleName",       "juce_jni");
        ndkSettings->add<GradleString> ("toolchain",        toolchain);
        ndkSettings->add<GradleString> ("stl", isClang ? "c++_static" :  "gnustl_static");

        addAllNdkCompilerSettings (ndkSettings);

        return ndkSettings;
    }

    void addAllNdkCompilerSettings (GradleObject* ndk) const
    {
        addNdkCppFlags (ndk);
        addNdkPreprocessorDefines (ndk);
        addNdkHeaderIncludePaths (ndk);
        addNdkLinkerFlags (ndk);
        addNdkLibraries (ndk);
    }

    void addNdkCppFlags (GradleObject* ndk) const
    {
        const char* alwaysUsedFlags[] = { "-fsigned-char", "-fexceptions", "-frtti", "-std=c++11", nullptr };
        StringArray cppFlags (alwaysUsedFlags);

        cppFlags.mergeArray (StringArray::fromTokens (getExtraCompilerFlagsString(), " ", ""));

        for (int i = 0; i < cppFlags.size(); ++i)
            ndk->add<GradleCppFlag> (cppFlags[i]);
    }

    void addNdkPreprocessorDefines (GradleObject* ndk) const
    {
        const auto& defines = getAllPreprocessorDefs();

        for (int i = 0; i < defines.size(); ++i)
            ndk->add<GradlePreprocessorDefine> ( defines.getAllKeys()[i], defines.getAllValues()[i]);
    }

    void addNdkHeaderIncludePaths (GradleObject* ndk) const
    {
        StringArray includePaths;

        for (const auto& cppFile : getAllCppFilesToBeIncludedWithPath())
            includePaths.addIfNotAlreadyThere (cppFile.getParentDirectory().toUnixStyle());

        for (const auto& path : includePaths)
            ndk->add<GradleHeaderIncludePath> (path);
    }

    Array<RelativePath> getAllCppFilesToBeIncludedWithPath() const
    {
        Array<RelativePath> cppFiles;

        struct NeedsToBeIncludedWithPathPredicate
        {
            bool operator() (const Project::Item& projectItem) const
            {
                return projectItem.shouldBeAddedToTargetProject() && ! projectItem.isModuleCode();
            }
        };

        for (const auto& group : getAllGroups())
            findAllProjectItemsWithPredicate (group, cppFiles, NeedsToBeIncludedWithPathPredicate());

        return cppFiles;
    }

    void addNdkLinkerFlags (GradleObject* ndk) const
    {
        const auto linkerFlags = StringArray::fromTokens (getExtraLinkerFlagsString(), " ", "");

        for (const auto& flag : linkerFlags)
            ndk->add<GradleLinkerFlag> (flag);

    }
    void addNdkLibraries (GradleObject* ndk) const
    {
        const char* requiredAndroidLibs[] = { "android", "EGL", "GLESv2", "log", nullptr };
        StringArray libs (requiredAndroidLibs);

        libs.addArray (StringArray::fromTokens(getExternalLibrariesString(), ";", ""));

        for (const auto& lib : libs)
            ndk->add<GradleLinkLibrary> (lib);
    }
#endif

    GradleObject* getAndroidSources() const
    {
        auto source = new GradleObject ("source");   // app source folder
        source->add<GradleStatement> ("exclude \"**/JuceModules/\"");

        auto jni = new GradleObject ("jni");         // all C++ sources for app
        jni->addChildObject (source);

        auto main = new GradleObject ("main");       // all sources for app
        main->addChildObject (jni);

        auto sources = new GradleObject ("android.sources"); // all sources
        sources->addChildObject (main);
        return sources;
    }

    GradleObject* getAndroidBuildConfigs() const
    {
        auto buildConfigs = new GradleObject ("android.buildTypes");

        for (ConstConfigIterator config (*this); config.next();)
            buildConfigs->addChildObject (getBuildConfig (*config));

        return buildConfigs;
    }

    GradleObject* getBuildConfig (const BuildConfiguration& config) const
    {
        const String configName (config.getName());

        // Note: at the moment, Android Studio only supports a "debug" and a "release"
        // build config, but no custom build configs like Projucer's other exporters do.
        if (configName != "Debug" && configName != "Release")
            throw SaveError ("Build configurations other than Debug and Release are not yet support for Android Studio");

        auto gradleConfig = new GradleObject (configName.toLowerCase());

        if (! config.isDebug())
            gradleConfig->add<GradleValue> ("signingConfig", "$(\"android.signingConfigs.releaseConfig\")");

        addConfigNdkSettings (gradleConfig, config);

        return gradleConfig;
    }

    void addConfigNdkSettings (GradleObject* buildConfig, const BuildConfiguration& config) const
    {
        auto ndkSettings = new GradleObject ("ndk.with");

        if (config.isDebug())
        {
            ndkSettings->add<GradleValue> ("debuggable", true);
            ndkSettings->add<GradleCppFlag> ("-g");
            ndkSettings->add<GradlePreprocessorDefine> ("DEBUG", "1");
            ndkSettings->add<GradlePreprocessorDefine> ("_DEBUG", "1");
        }
        else
        {
            ndkSettings->add<GradlePreprocessorDefine> ("NDEBUG", "1");
        }

        ndkSettings->add<GradleCppFlag> ("-O" + config.getGCCOptimisationFlag());

        for (const auto& path : getHeaderSearchPaths (config))
            ndkSettings->add<GradleHeaderIncludePath> (path);

        for (const auto& path : config.getLibrarySearchPaths())
            ndkSettings->add<GradleLibrarySearchPath> (path);

        ndkSettings->add<GradlePreprocessorDefine> ("JUCE_ANDROID", "1");
        ndkSettings->add<GradlePreprocessorDefine> ("JUCE_ANDROID_API_VERSION", androidMinimumSDK.get());
        ndkSettings->add<GradlePreprocessorDefine> ("JUCE_ANDROID_ACTIVITY_CLASSNAME", getJNIActivityClassName().replaceCharacter ('/', '_'));
        ndkSettings->add<GradlePreprocessorDefine> ("JUCE_ANDROID_ACTIVITY_CLASSPATH","\\\"" + androidActivityClass.get().replaceCharacter('.', '/') + "\\\"");

        const auto defines = config.getAllPreprocessorDefs();
        for (int i = 0; i < defines.size(); ++i)
            ndkSettings->add<GradlePreprocessorDefine> (defines.getAllKeys()[i], defines.getAllValues()[i]);

        buildConfig->addChildObject (ndkSettings);
    }

    StringArray getHeaderSearchPaths (const BuildConfiguration& config) const
    {
        StringArray paths (extraSearchPaths);
        paths.addArray (config.getHeaderSearchPaths());
        paths = getCleanedStringArray (paths);
        return paths;
    }

    GradleObject* getAndroidSigningConfigs() const
    {
        auto releaseConfig = new GradleObject ("create(\"releaseConfig\")");

        releaseConfig->add<GradleFilePath>  ("storeFile",     androidKeyStore.get());
        releaseConfig->add<GradleString>    ("storePassword", androidKeyStorePass.get());
        releaseConfig->add<GradleString>    ("keyAlias",      androidKeyAlias.get());
        releaseConfig->add<GradleString>    ("keyPassword",   androidKeyAliasPass.get());
        releaseConfig->add<GradleString>    ("storeType",     "jks");

        auto signingConfigs = new GradleObject ("android.signingConfigs");

        signingConfigs->addChildObject (releaseConfig);
        // Note: no need to add a debugConfig, Android Studio will use debug.keystore by default

        return signingConfigs;
    }

    GradleObject* getAndroidProductFlavours() const
    {
        auto flavours = new GradleObject ("android.productFlavors");

        StringArray architectures (StringArray::fromTokens (getABIs<AndroidStudioBuildConfiguration> (true),  " ", ""));
        architectures.mergeArray  (StringArray::fromTokens (getABIs<AndroidStudioBuildConfiguration> (false), " ", ""));

        if (architectures.size() == 0)
            throw SaveError ("Can't build for no architectures!");

        for (int i = 0; i < architectures.size(); ++i)
        {
            String arch (architectures[i].trim());

            if ((arch).isEmpty())
                continue;

            flavours->addChildObject (getGradleProductFlavourForArch (arch));
        }

        return flavours;
    }

    GradleObject* getGradleProductFlavourForArch (const String& arch) const
    {
        auto flavour = new GradleObject ("create(\"" + arch + "\")");
        flavour->add<GradleStatement> ("ndk.abiFilters.add(\"" + arch + "\")");
        return flavour;
    }
    //==============================================================================
    String getLocalPropertiesFileContent() const
    {
        String props;

        props << "ndk.dir=" << sanitisePath (ndkPath.toString()) << newLine
              << "sdk.dir=" << sanitisePath (sdkPath.toString()) << newLine;

        return props;
    }

    String getGradleWrapperPropertiesFileContent() const
    {
        String props;

        props << "distributionUrl=https\\://services.gradle.org/distributions/gradle-"
              << gradleVersion.get() << "-all.zip";

        return props;
    }

    //==============================================================================
    void writeStringsXML (const File& folder) const
    {
        XmlElement strings ("resources");
        XmlElement* resourceName = strings.createNewChildElement ("string");

        resourceName->setAttribute ("name", "app_name");
        resourceName->addTextElement (projectName);

        writeXmlOrThrow (strings, folder.getChildFile ("app/src/main/res/values/string.xml"), "utf-8", 100, true);
    }

    //==============================================================================
    void writeAndroidManifest (const File& folder) const
    {
        ScopedPointer<XmlElement> manifest (createManifestXML());

        writeXmlOrThrow (*manifest, folder.getChildFile ("app/src/main/AndroidManifest.xml"), "utf-8", 100, true);
    }

    //==============================================================================
    //==============================================================================
    void createBaseExporterProperties (PropertyListBuilder& props)
    {
        static const char* orientations[] = { "Portrait and Landscape", "Portrait", "Landscape", nullptr };
        static const char* orientationValues[] = { "unspecified", "portrait", "landscape", nullptr };

        props.add (new ChoicePropertyComponent (androidScreenOrientation.getPropertyAsValue(), "Screen orientation", StringArray (orientations), Array<var> (orientationValues)),
                   "The screen orientations that this app should support");

        props.add (new TextWithDefaultPropertyComponent<String> (androidActivityClass, "Android Activity class name", 256),
                   "The full java class name to use for the app's Activity class.");

        props.add (new TextPropertyComponent (androidActivitySubClassName.getPropertyAsValue(), "Android Activity sub-class name", 256, false),
                   "If not empty, specifies the Android Activity class name stored in the app's manifest. "
                   "Use this if you would like to use your own Android Activity sub-class.");

        props.add (new TextWithDefaultPropertyComponent<String> (androidVersionCode, "Android Version Code", 32),
                   "An integer value that represents the version of the application code, relative to other versions.");

        props.add (new DependencyPathPropertyComponent (project.getFile().getParentDirectory(), sdkPath, "Android SDK Path"),
                   "The path to the Android SDK folder on the target build machine");

        props.add (new DependencyPathPropertyComponent (project.getFile().getParentDirectory(), ndkPath, "Android NDK Path"),
                   "The path to the Android NDK folder on the target build machine");

        props.add (new TextWithDefaultPropertyComponent<String> (androidMinimumSDK, "Minimum SDK version", 32),
                   "The number of the minimum version of the Android SDK that the app requires");
    }

    //==============================================================================
    void createManifestExporterProperties (PropertyListBuilder& props)
    {
        props.add (new BooleanPropertyComponent (androidInternetNeeded.getPropertyAsValue(), "Internet Access", "Specify internet access permission in the manifest"),
                   "If enabled, this will set the android.permission.INTERNET flag in the manifest.");

        props.add (new BooleanPropertyComponent (androidMicNeeded.getPropertyAsValue(), "Audio Input Required", "Specify audio record permission in the manifest"),
                   "If enabled, this will set the android.permission.RECORD_AUDIO flag in the manifest.");

        props.add (new BooleanPropertyComponent (androidBluetoothNeeded.getPropertyAsValue(), "Bluetooth permissions Required", "Specify bluetooth permission (required for Bluetooth MIDI)"),
                   "If enabled, this will set the android.permission.BLUETOOTH and  android.permission.BLUETOOTH_ADMIN flag in the manifest. This is required for Bluetooth MIDI on Android.");

        props.add (new TextPropertyComponent (androidOtherPermissions.getPropertyAsValue(), "Custom permissions", 2048, false),
                   "A space-separated list of other permission flags that should be added to the manifest.");
    }

    //==============================================================================
    void createCodeSigningExporterProperties (PropertyListBuilder& props)
    {
        props.add (new TextWithDefaultPropertyComponent<String> (androidKeyStore, "Key Signing: key.store", 2048),
                   "The key.store value, used when signing the package.");

        props.add (new TextWithDefaultPropertyComponent<String> (androidKeyStorePass, "Key Signing: key.store.password", 2048),
                   "The key.store password, used when signing the package.");

        props.add (new TextWithDefaultPropertyComponent<String> (androidKeyAlias, "Key Signing: key.alias", 2048),
                   "The key.alias value, used when signing the package.");

        props.add (new TextWithDefaultPropertyComponent<String> (androidKeyAliasPass, "Key Signing: key.alias.password", 2048),
                   "The key.alias password, used when signing the package.");
    }

    //==============================================================================
    void createOtherExporterProperties (PropertyListBuilder& props)
    {
        props.add (new TextPropertyComponent (androidTheme.getPropertyAsValue(), "Android Theme", 256, false),
                   "E.g. @android:style/Theme.NoTitleBar or leave blank for default");
    }

    //==============================================================================
    String createDefaultClassName() const
    {
        String s (project.getBundleIdentifier().toString().toLowerCase());

        if (s.length() > 5
            && s.containsChar ('.')
            && s.containsOnly ("abcdefghijklmnopqrstuvwxyz_.")
            && ! s.startsWithChar ('.'))
        {
            if (! s.endsWithChar ('.'))
                s << ".";
        }
        else
        {
            s = "com.yourcompany.";
        }

        return s + CodeHelpers::makeValidIdentifier (project.getProjectFilenameRoot(), false, true, false);
    }

    void initialiseDependencyPathValues()
    {
        sdkPath.referTo (Value (new DependencyPathValueSource (getSetting (Ids::androidSDKPath),
                                                               Ids::androidSDKPath, TargetOS::getThisOS())));

        ndkPath.referTo (Value (new DependencyPathValueSource (getSetting (Ids::androidNDKPath),
                                                               Ids::androidNDKPath, TargetOS::getThisOS())));
    }

    void copyActivityJavaFiles (const OwnedArray<LibraryModule>& modules, const File& targetFolder, const String& package) const
    {
        const String className (getActivityName());

        if (className.isEmpty())
            throw SaveError ("Invalid Android Activity class name: " + androidActivityClass.get());

        createDirectoryOrThrow (targetFolder);

        LibraryModule* const coreModule = getCoreModule (modules);

        if (coreModule != nullptr)
        {
            File javaDestFile (targetFolder.getChildFile (className + ".java"));

            File javaSourceFolder (coreModule->getFolder().getChildFile ("native")
                                                          .getChildFile ("java"));

            String juceMidiCode, juceMidiImports, juceRuntimePermissionsCode;

            juceMidiImports << newLine;

            if (androidMinimumSDK.get().getIntValue() >= 23)
            {
                File javaAndroidMidi (javaSourceFolder.getChildFile ("AndroidMidi.java"));
                File javaRuntimePermissions (javaSourceFolder.getChildFile ("AndroidRuntimePermissions.java"));

                juceMidiImports << "import android.media.midi.*;" << newLine
                                << "import android.bluetooth.*;" << newLine
                                << "import android.bluetooth.le.*;" << newLine;

                juceMidiCode = javaAndroidMidi.loadFileAsString().replace ("JuceAppActivity", className);

                juceRuntimePermissionsCode = javaRuntimePermissions.loadFileAsString().replace ("JuceAppActivity", className);
            }
            else
            {
                juceMidiCode = javaSourceFolder.getChildFile ("AndroidMidiFallback.java")
                                   .loadFileAsString()
                                   .replace ("JuceAppActivity", className);
            }

            File javaSourceFile (javaSourceFolder.getChildFile ("JuceAppActivity.java"));
            StringArray javaSourceLines (StringArray::fromLines (javaSourceFile.loadFileAsString()));

            {
                MemoryOutputStream newFile;

                for (int i = 0; i < javaSourceLines.size(); ++i)
                {
                    const String& line = javaSourceLines[i];

                    if (line.contains ("$$JuceAndroidMidiImports$$"))
                        newFile << juceMidiImports;
                    else if (line.contains ("$$JuceAndroidMidiCode$$"))
                        newFile << juceMidiCode;
                    else if (line.contains ("$$JuceAndroidRuntimePermissionsCode$$"))
                        newFile << juceRuntimePermissionsCode;
                    else
                        newFile << line.replace ("JuceAppActivity", className)
                                       .replace ("package com.juce;", "package " + package + ";") << newLine;
                }

                javaSourceLines = StringArray::fromLines (newFile.toString());
            }

            while (javaSourceLines.size() > 2
                    && javaSourceLines[javaSourceLines.size() - 1].trim().isEmpty()
                    && javaSourceLines[javaSourceLines.size() - 2].trim().isEmpty())
                javaSourceLines.remove (javaSourceLines.size() - 1);

            overwriteFileIfDifferentOrThrow (javaDestFile, javaSourceLines.joinIntoString (newLine));
        }
    }

    String getActivityName() const
    {
        return androidActivityClass.get().fromLastOccurrenceOf (".", false, false);
    }

    String getActivitySubClassName() const
    {
        String activityPath = androidActivitySubClassName.get();

        return (activityPath.isEmpty()) ? getActivityName() : activityPath.fromLastOccurrenceOf (".", false, false);
    }

    String getActivityClassPackage() const
    {
        return androidActivityClass.get().upToLastOccurrenceOf (".", false, false);
    }

    String getJNIActivityClassName() const
    {
        return androidActivityClass.get().replaceCharacter ('.', '/');
    }

    static LibraryModule* getCoreModule (const OwnedArray<LibraryModule>& modules)
    {
        for (int i = modules.size(); --i >= 0;)
            if (modules.getUnchecked(i)->getID() == "juce_core")
                return modules.getUnchecked(i);

        return nullptr;
    }

    StringArray getPermissionsRequired() const
    {
        StringArray s;
        s.addTokens (androidOtherPermissions.get(), ", ", "");

        if (androidInternetNeeded.get())
            s.add ("android.permission.INTERNET");

        if (androidMicNeeded.get())
            s.add ("android.permission.RECORD_AUDIO");

        if (androidBluetoothNeeded.get())
        {
            s.add ("android.permission.BLUETOOTH");
            s.add ("android.permission.BLUETOOTH_ADMIN");
            s.add ("android.permission.ACCESS_COARSE_LOCATION");
        }

        return getCleanedStringArray (s);
    }

    template <typename PredicateT>
    void findAllProjectItemsWithPredicate (const Project::Item& projectItem, Array<RelativePath>& results, const PredicateT& predicate) const
    {
        if (projectItem.isGroup())
        {
            for (int i = 0; i < projectItem.getNumChildren(); ++i)
                findAllProjectItemsWithPredicate (projectItem.getChild(i), results, predicate);
        }
        else
        {
            if (predicate (projectItem))
                results.add (RelativePath (projectItem.getFile(), getTargetFolder(), RelativePath::buildTargetFolder));
        }
    }

    void writeIcon (const File& file, const Image& im) const
    {
        if (im.isValid())
        {
            createDirectoryOrThrow (file.getParentDirectory());

            PNGImageFormat png;
            MemoryOutputStream mo;

            if (! png.writeImageToStream (im, mo))
                throw SaveError ("Can't generate Android icon file");

            overwriteFileIfDifferentOrThrow (file, mo);
        }
    }

    void writeIcons (const File& folder) const
    {
        ScopedPointer<Drawable> bigIcon (getBigIcon());
        ScopedPointer<Drawable> smallIcon (getSmallIcon());

        if (bigIcon != nullptr && smallIcon != nullptr)
        {
            const int step = jmax (bigIcon->getWidth(), bigIcon->getHeight()) / 8;
            writeIcon (folder.getChildFile ("drawable-xhdpi/icon.png"), getBestIconForSize (step * 8, false));
            writeIcon (folder.getChildFile ("drawable-hdpi/icon.png"),  getBestIconForSize (step * 6, false));
            writeIcon (folder.getChildFile ("drawable-mdpi/icon.png"),  getBestIconForSize (step * 4, false));
            writeIcon (folder.getChildFile ("drawable-ldpi/icon.png"),  getBestIconForSize (step * 3, false));
        }
        else if (Drawable* icon = bigIcon != nullptr ? bigIcon : smallIcon)
        {
            writeIcon (folder.getChildFile ("drawable-mdpi/icon.png"), rescaleImageForIcon (*icon, icon->getWidth()));
        }
    }

    template <typename BuildConfigType>
    String getABIs (bool forDebug) const
    {
        for (ConstConfigIterator config (*this); config.next();)
        {
            const BuildConfigType& androidConfig = dynamic_cast<const BuildConfigType&> (*config);

            if (config->isDebug() == forDebug)
                return androidConfig.getArchitectures();
        }

        return String();
    }

    //==============================================================================
    XmlElement* createManifestXML() const
    {
        XmlElement* manifest = new XmlElement ("manifest");

        manifest->setAttribute ("xmlns:android", "http://schemas.android.com/apk/res/android");
        manifest->setAttribute ("android:versionCode", androidVersionCode.get());
        manifest->setAttribute ("android:versionName",  project.getVersionString());
        manifest->setAttribute ("package", getActivityClassPackage());

        XmlElement* screens = manifest->createNewChildElement ("supports-screens");
        screens->setAttribute ("android:smallScreens", "true");
        screens->setAttribute ("android:normalScreens", "true");
        screens->setAttribute ("android:largeScreens", "true");
        //screens->setAttribute ("android:xlargeScreens", "true");
        screens->setAttribute ("android:anyDensity", "true");

        XmlElement* sdk = manifest->createNewChildElement ("uses-sdk");
        sdk->setAttribute ("android:minSdkVersion", androidMinimumSDK.get());
        sdk->setAttribute ("android:targetSdkVersion", androidMinimumSDK.get());

        {
            const StringArray permissions (getPermissionsRequired());

            for (int i = permissions.size(); --i >= 0;)
                manifest->createNewChildElement ("uses-permission")->setAttribute ("android:name", permissions[i]);
        }

        if (project.getModules().isModuleEnabled ("juce_opengl"))
        {
            XmlElement* feature = manifest->createNewChildElement ("uses-feature");
            feature->setAttribute ("android:glEsVersion", "0x00020000");
            feature->setAttribute ("android:required", "true");
        }

        XmlElement* app = manifest->createNewChildElement ("application");
        app->setAttribute ("android:label", "@string/app_name");

        if (androidTheme.get().isNotEmpty())
            app->setAttribute ("android:theme", androidTheme.get());

        {
            ScopedPointer<Drawable> bigIcon (getBigIcon()), smallIcon (getSmallIcon());

            if (bigIcon != nullptr || smallIcon != nullptr)
                app->setAttribute ("android:icon", "@drawable/icon");
        }

        if (androidMinimumSDK.get().getIntValue() >= 11)
            app->setAttribute ("android:hardwareAccelerated", "false"); // (using the 2D acceleration slows down openGL)

        XmlElement* act = app->createNewChildElement ("activity");
        act->setAttribute ("android:name", getActivitySubClassName());
        act->setAttribute ("android:label", "@string/app_name");
        act->setAttribute ("android:configChanges", "keyboardHidden|orientation|screenSize");
        act->setAttribute ("android:screenOrientation", androidScreenOrientation.get());

        XmlElement* intent = act->createNewChildElement ("intent-filter");
        intent->createNewChildElement ("action")->setAttribute ("android:name", "android.intent.action.MAIN");
        intent->createNewChildElement ("category")->setAttribute ("android:name", "android.intent.category.LAUNCHER");

        return manifest;
    }

    //==============================================================================
    Value sdkPath, ndkPath;
    const File androidStudioExecutable;

    JUCE_DECLARE_NON_COPYABLE (AndroidStudioProjectExporter)
};

ProjectExporter* createAndroidStudioExporter (Project& p, const ValueTree& t)
{
    return new AndroidStudioProjectExporter (p, t);
}

ProjectExporter* createAndroidStudioExporterForSetting (Project& p, const ValueTree& t)
{
    return AndroidStudioProjectExporter::createForSettings (p, t);
}
