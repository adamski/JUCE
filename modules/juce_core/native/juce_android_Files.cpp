/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD) \
  METHOD (constructor, "<init>",     "(Landroid/content/Context;Landroid/media/MediaScannerConnection$MediaScannerConnectionClient;)V") \
  METHOD (connect,     "connect",    "()V") \
  METHOD (disconnect,  "disconnect", "()V") \
  METHOD (scanFile,    "scanFile",   "(Ljava/lang/String;Ljava/lang/String;)V") \

DECLARE_JNI_CLASS (MediaScannerConnection, "android/media/MediaScannerConnection");
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD) \
 METHOD (query,            "query",            "(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)Landroid/database/Cursor;") \
 METHOD (openInputStream,  "openInputStream",  "(Landroid/net/Uri;)Ljava/io/InputStream;") \
 METHOD (openOutputStream, "openOutputStream", "(Landroid/net/Uri;)Ljava/io/OutputStream;")

DECLARE_JNI_CLASS (ContentResolver, "android/content/ContentResolver");
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD) \
 METHOD (moveToFirst,     "moveToFirst",     "()Z") \
 METHOD (getColumnIndex,  "getColumnIndex",  "(Ljava/lang/String;)I") \
 METHOD (getString,       "getString",       "(I)Ljava/lang/String;") \
 METHOD (close,           "close",           "()V") \

DECLARE_JNI_CLASS (AndroidCursor, "android/database/Cursor");
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD) \
 STATICMETHOD (getExternalStorageDirectory, "getExternalStorageDirectory", "()Ljava/io/File;") \
 STATICMETHOD (getExternalStoragePublicDirectory, "getExternalStoragePublicDirectory", "(Ljava/lang/String;)Ljava/io/File;") \

DECLARE_JNI_CLASS (AndroidEnvironment, "android/os/Environment");
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD) \
 METHOD (close, "close", "()V") \
 METHOD (flush, "flush", "()V") \
 METHOD (write, "write", "([BII)V")

DECLARE_JNI_CLASS (AndroidOutputStream, "java/io/OutputStream");
#undef JNI_CLASS_MEMBERS

//==============================================================================
struct AndroidContentUriResolver
{
public:
    static LocalRef<jobject> getStreamForContentUri (const URL& url, bool inputStream)
    {
        // only use this method for content URIs
        jassert (url.getScheme() == "content");
        auto* env = getEnv();

        LocalRef<jobject> contentResolver (android.bridge.callObjectMethod (JuceBridge.getContentResolver));

        if (contentResolver)
            return LocalRef<jobject> ((env->CallObjectMethod (contentResolver.get(),
                                                              inputStream ? ContentResolver.openInputStream
                                                                          : ContentResolver.openOutputStream,
                                                              urlToUri (url).get())));

        return LocalRef<jobject>();
    }

    static File getLocalFileFromContentUri (const URL& url)
    {
        // only use this method for content URIs
        jassert (url.getScheme() == "content");

        auto authority  = url.getDomain();
        auto documentId = URL::removeEscapeChars (url.getSubPath().fromFirstOccurrenceOf ("/", false, false));
        auto tokens = StringArray::fromTokens (documentId, ":", "");

        if (authority == "com.android.externalstorage.documents")
        {
            auto storageId  = tokens[0];
            auto subpath    = tokens[1];

            auto storagePath = getStorageDevicePath (storageId);

            if (storagePath != File())
                return storagePath.getChildFile (subpath);
        }
        else if (authority == "com.android.providers.downloads.documents")
        {
            auto type       = tokens[0];
            auto downloadId = tokens[1];

            if (type.equalsIgnoreCase ("raw"))
            {
                return File (downloadId);
            }
            else if (type.equalsIgnoreCase ("downloads"))
            {
                auto subDownloadPath = url.getSubPath().fromFirstOccurrenceOf ("tree/downloads", false, false);
                return File (getWellKnownFolder ("Download").getFullPathName() + "/" + subDownloadPath);
            }
            else
            {
                return getLocalFileFromContentUri (URL ("content://downloads/public_downloads/" + documentId));
            }
        }
        else if (authority == "com.android.providers.media.documents" && documentId.isNotEmpty())
        {
            auto type    = tokens[0];
            auto mediaId = tokens[1];

            if (type == "image")
                type = "images";

            return getCursorDataColumn (URL ("content://media/external/" + type + "/media"),
                                        "_id=?", StringArray {mediaId});
        }

        return getCursorDataColumn (url);
    }

    static String getFileNameFromContentUri (const URL& url)
    {
        auto uri = urlToUri (url);
        auto* env = getEnv();
        LocalRef<jobject> contentResolver (android.bridge.callObjectMethod (JuceBridge.getContentResolver));

        if (contentResolver == 0)
            return {};

        auto filename = getStringUsingDataColumn ("_display_name", env, uri, contentResolver);

        // Fallback to "_data" column
        if (filename.isEmpty())
        {
            auto path = getStringUsingDataColumn ("_data", env, uri, contentResolver);
            filename = path.fromLastOccurrenceOf ("/", false, true);
        }

        return filename;
    }

private:
    //==============================================================================
    static String getCursorDataColumn (const URL& url, const String& selection = {},
                                       const StringArray& selectionArgs = {})
    {
        auto uri = urlToUri (url);
        auto* env = getEnv();
        LocalRef<jobject> contentResolver (android.bridge.callObjectMethod (JuceBridge.getContentResolver));

        if (contentResolver)
        {
            LocalRef<jstring> columnName (javaString ("_data"));
            LocalRef<jobjectArray> projection (env->NewObjectArray (1, JavaString, columnName.get()));

            LocalRef<jobjectArray> args;

            if (selection.isNotEmpty())
            {
                args = LocalRef<jobjectArray> (env->NewObjectArray (selectionArgs.size(), JavaString, javaString ("").get()));

                for (int i = 0; i < selectionArgs.size(); ++i)
                    env->SetObjectArrayElement (args.get(), i, javaString (selectionArgs[i]).get());
            }

            LocalRef<jstring> jSelection (selection.isNotEmpty() ? javaString (selection) : LocalRef<jstring>());
            LocalRef<jobject> cursor (env->CallObjectMethod (contentResolver.get(), ContentResolver.query,
                                                             uri.get(), projection.get(), jSelection.get(),
                                                             args.get(), nullptr));

            if (cursor)
            {
                if (env->CallBooleanMethod (cursor.get(), AndroidCursor.moveToFirst) != 0)
                {
                    auto columnIndex = env->CallIntMethod (cursor.get(), AndroidCursor.getColumnIndex, columnName.get());

                    if (columnIndex >= 0)
                    {
                        LocalRef<jstring> value ((jstring) env->CallObjectMethod (cursor.get(), AndroidCursor.getString, columnIndex));

                        if (value)
                            return juceString (value.get());
                    }
                }

                env->CallVoidMethod (cursor.get(), AndroidCursor.close);
            }
        }

        return {};
    }

    //==============================================================================
    static File getWellKnownFolder (const String& folderId)
    {
        auto* env = getEnv();
        LocalRef<jobject> downloadFolder (env->CallStaticObjectMethod (AndroidEnvironment,
                                                                       AndroidEnvironment.getExternalStoragePublicDirectory,
                                                                       javaString (folderId).get()));

        return (downloadFolder ? juceFile (downloadFolder) : File());
    }

    //==============================================================================
    static File getStorageDevicePath (const String& storageId)
    {
        // check for the primary alias
        if (storageId == "primary")
            return getPrimaryStorageDirectory();

        auto storageDevices = getSecondaryStorageDirectories();

        for (auto storageDevice : storageDevices)
            if (getStorageIdForMountPoint (storageDevice) == storageId)
                return storageDevice;

        return {};
    }

    static File getPrimaryStorageDirectory()
    {
        auto* env = getEnv();
        return juceFile (LocalRef<jobject> (env->CallStaticObjectMethod (AndroidEnvironment, AndroidEnvironment.getExternalStorageDirectory)));
    }

    static Array<File> getSecondaryStorageDirectories()
    {
        Array<File> results;

        if (getSDKVersion() >= 19)
        {
            auto* env = getEnv();
            static jmethodID m = (env->GetMethodID (JuceBridge, "getExternalFilesDirs",
                                                    "(Ljava/lang/String;)[Ljava/io/File;"));
            if (m == 0)
                return {};

            auto paths = convertFileArray (LocalRef<jobject> (android.bridge.callObjectMethod (m, nullptr)));

            for (auto path : paths)
                results.add (getMountPointForFile (path));
        }
        else
        {
            // on older SDKs other external storages are located "next" to the primary
            // storage mount point
            auto mountFolder = getMountPointForFile (getPrimaryStorageDirectory())
                                    .getParentDirectory();

            // don't include every folder. Only folders which are actually mountpoints
            juce_statStruct info;
            if (! juce_stat (mountFolder.getFullPathName(), info))
                return {};

            auto rootFsDevice = info.st_dev;
            DirectoryIterator iter (mountFolder, false, "*", File::findDirectories);

            while (iter.next())
            {
                auto candidate = iter.getFile();

                if (juce_stat (candidate.getFullPathName(), info)
                      && info.st_dev != rootFsDevice)
                    results.add (candidate);
            }

        }

        return results;
    }

    //==============================================================================
    static String getStorageIdForMountPoint (const File& mountpoint)
    {
        // currently this seems to work fine, but something
        // more intelligent may be needed in the future
        return mountpoint.getFileName();
    }

    static File getMountPointForFile (const File& file)
    {
        juce_statStruct info;

        if (juce_stat (file.getFullPathName(), info))
        {
            auto dev  = info.st_dev;
            File mountPoint = file;

            for (;;)
            {
                auto parent = mountPoint.getParentDirectory();

                if (parent == mountPoint)
                    break;

                juce_stat (parent.getFullPathName(), info);

                if (info.st_dev != dev)
                    break;

                mountPoint = parent;
            }

            return mountPoint;
        }

        return {};
    }

    //==============================================================================
    static Array<File> convertFileArray (LocalRef<jobject> obj)
    {
        auto* env = getEnv();
        int n = (int) env->GetArrayLength ((jobjectArray) obj.get());
        Array<File> files;

        for (int i = 0; i < n; ++i)
            files.add (juceFile (LocalRef<jobject> (env->GetObjectArrayElement ((jobjectArray) obj.get(),
                                                                                 (jsize) i))));

        return files;
    }

    static File juceFile (LocalRef<jobject> obj)
    {
        auto* env = getEnv();

        if (env->IsInstanceOf (obj.get(), JavaFile) != 0)
            return File (juceString (LocalRef<jstring> ((jstring) env->CallObjectMethod (obj.get(),
                                                        JavaFile.getAbsolutePath))));

        return {};
    }

    //==============================================================================
    static int getSDKVersion()
    {
        static int sdkVersion
            = getEnv()->CallStaticIntMethod (JuceBridge,
                                             JuceBridge.getAndroidSDKVersion);

        return sdkVersion;
    }

    static LocalRef<jobject> urlToUri (const URL& url)
    {
        return LocalRef<jobject> (getEnv()->CallStaticObjectMethod (AndroidUri, AndroidUri.parse,
                                                                    javaString (url.toString (true)).get()));
    }

    //==============================================================================
    static String getStringUsingDataColumn (const String& columnNameToUse, JNIEnv* env,
                                            const LocalRef<jobject>& uri,
                                            const LocalRef<jobject>& contentResolver)
    {
        LocalRef<jstring> columnName (javaString (columnNameToUse));
        LocalRef<jobjectArray> projection (env->NewObjectArray (1, JavaString, columnName.get()));

        LocalRef<jobject> cursor (env->CallObjectMethod (contentResolver.get(), ContentResolver.query,
                                                         uri.get(), projection.get(), nullptr,
                                                         nullptr, nullptr));

        if (cursor == 0)
            return {};

        String fileName;

        if (env->CallBooleanMethod (cursor.get(), AndroidCursor.moveToFirst) != 0)
        {
            auto columnIndex = env->CallIntMethod (cursor.get(), AndroidCursor.getColumnIndex, columnName.get());

            if (columnIndex >= 0)
            {
                LocalRef<jstring> value ((jstring) env->CallObjectMethod (cursor.get(), AndroidCursor.getString, columnIndex));

                if (value)
                    fileName = juceString (value.get());

            }
        }

        env->CallVoidMethod (cursor.get(), AndroidCursor.close);

        return fileName;
    }
};

//==============================================================================
struct AndroidContentUriOutputStream :  public OutputStream
{
    AndroidContentUriOutputStream (LocalRef<jobject>&& outputStream)
        : stream (outputStream)
    {
    }

    ~AndroidContentUriOutputStream()
    {
        stream.callVoidMethod (AndroidOutputStream.close);
    }

    void flush() override
    {
        stream.callVoidMethod (AndroidOutputStream.flush);
    }

    bool setPosition (int64 newPos) override
    {
        return (newPos == pos);
    }

    int64 getPosition() override
    {
        return pos;
    }

    bool write (const void* dataToWrite, size_t numberOfBytes) override
    {
        if (numberOfBytes == 0)
            return true;

        JNIEnv* env = getEnv();

        jbyteArray javaArray = env->NewByteArray ((jsize) numberOfBytes);
        env->SetByteArrayRegion (javaArray, 0, (jsize) numberOfBytes, (const jbyte*) dataToWrite);

        stream.callVoidMethod (AndroidOutputStream.write, javaArray, 0, (jint) numberOfBytes);
        env->DeleteLocalRef (javaArray);

        pos += static_cast<int64> (numberOfBytes);
        return true;
    }

    GlobalRef stream;
    int64 pos = 0;
};

OutputStream* juce_CreateContentURIOutputStream (const URL& url)
{
    auto stream = AndroidContentUriResolver::getStreamForContentUri (url, false);

    return (stream.get() != 0 ? new AndroidContentUriOutputStream (static_cast<LocalRef<jobject>&&> (stream)) : nullptr);
}

//==============================================================================
class MediaScannerConnectionClient : public AndroidInterfaceImplementer
{
public:
    virtual void onMediaScannerConnected() = 0;
    virtual void onScanCompleted() = 0;

private:
    jobject invoke (jobject proxy, jobject method, jobjectArray args) override
    {
        auto* env = getEnv();

        auto methodName = juceString ((jstring) env->CallObjectMethod (method, JavaMethod.getName));

        if (methodName == "onMediaScannerConnected")
        {
            onMediaScannerConnected();
            return nullptr;
        }
        else if (methodName == "onScanCompleted")
        {
            onScanCompleted();
            return nullptr;
        }

        return AndroidInterfaceImplementer::invoke (proxy, method, args);
    }
};

//==============================================================================
bool File::isOnCDRomDrive() const
{
    return false;
}

bool File::isOnHardDisk() const
{
    return true;
}

bool File::isOnRemovableDrive() const
{
    return false;
}

String File::getVersion() const
{
    return {};
}

static File getSpecialFile (jmethodID type)
{
    return File (juceString (LocalRef<jstring> ((jstring) getEnv()->CallStaticObjectMethod (JuceBridge, type))));
}

File File::getSpecialLocation (const SpecialLocationType type)
{
    switch (type)
    {
        case userHomeDirectory:
        case userApplicationDataDirectory:
        case userDesktopDirectory:
        case commonApplicationDataDirectory:
            return File (android.appDataDir);

        case userDocumentsDirectory:
        case commonDocumentsDirectory:  return getSpecialFile (JuceBridge.getDocumentsFolder);
        case userPicturesDirectory:     return getSpecialFile (JuceBridge.getPicturesFolder);
        case userMusicDirectory:        return getSpecialFile (JuceBridge.getMusicFolder);
        case userMoviesDirectory:       return getSpecialFile (JuceBridge.getMoviesFolder);

        case globalApplicationsDirectory:
            return File ("/system/app");

        case tempDirectory:
        {
            File tmp = File (android.appDataDir).getChildFile (".temp");
            tmp.createDirectory();
            return File (tmp.getFullPathName());
        }

        case invokedExecutableFile:
        case currentExecutableFile:
        case currentApplicationFile:
        case hostApplicationPath:
            return juce_getExecutableFile();

        default:
            jassertfalse; // unknown type?
            break;
    }

    return {};
}

bool File::moveToTrash() const
{
    if (! exists())
        return true;

    // TODO
    return false;
}

JUCE_API bool JUCE_CALLTYPE Process::openDocument (const String& fileName, const String&)
{
    const LocalRef<jstring> t (javaString (fileName));
    android.bridge.callVoidMethod (JuceBridge.launchURL, t.get());
    return true;
}

void File::revealToUser() const
{
}

//==============================================================================
class SingleMediaScanner : public MediaScannerConnectionClient
{
public:
    SingleMediaScanner (const String& filename)
        : msc (getEnv()->NewObject (MediaScannerConnection,
                                    MediaScannerConnection.constructor,
                                    android.bridge.callObjectMethod(JuceBridge.getActivityContext),
                                    CreateJavaInterface (this, "android/media/MediaScannerConnection$MediaScannerConnectionClient").get())),
          file (filename)
    {
        getEnv()->CallVoidMethod (msc.get(), MediaScannerConnection.connect);
    }

    void onMediaScannerConnected() override
    {
        auto* env = getEnv();

        env->CallVoidMethod (msc.get(), MediaScannerConnection.scanFile, javaString (file).get(), 0);
    }

    void onScanCompleted() override
    {
        getEnv()->CallVoidMethod (msc.get(), MediaScannerConnection.disconnect);
    }

private:
    GlobalRef msc;
    String file;
};

void FileOutputStream::flushInternal()
{
    if (fileHandle != 0)
    {
        if (fsync (getFD (fileHandle)) == -1)
            status = getResultForErrno();

        // This stuff tells the OS to asynchronously update the metadata
        // that the OS has cached about the file - this metadata is used
        // when the device is acting as a USB drive, and unless it's explicitly
        // refreshed, it'll get out of step with the real file.
        new SingleMediaScanner (file.getFullPathName());
    }
}

} // namespace juce
