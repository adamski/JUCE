/*
  ==============================================================================

   This file is part of the juce_core module of the JUCE library.
   Copyright (c) 2015 - ROLI Ltd.

   Permission to use, copy, modify, and/or distribute this software for any purpose with
   or without fee is hereby granted, provided that the above copyright notice and this
   permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
   NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
   DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
   IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

   ------------------------------------------------------------------------------

   NOTE! This permissive ISC license applies ONLY to files within the juce_core module!
   All other JUCE modules are covered by a dual GPL/commercial license, so if you are
   using any other modules, be sure to check that you also comply with their license.

   For more details, visit www.juce.com

  ==============================================================================
*/

//==============================================================================
#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD) \
 METHOD (constructor, "<init>", "()V") \
 METHOD (toString, "toString", "()Ljava/lang/String;") \

DECLARE_JNI_CLASS (StringBuffer, "java/lang/StringBuffer");
#undef JNI_CLASS_MEMBERS

//==============================================================================
#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD) \
 METHOD (release, "release", "()V") \
 METHOD (read, "read", "([BI)I") \
 METHOD (getPosition, "getPosition", "()J") \
 METHOD (getTotalLength, "getTotalLength", "()J") \
 METHOD (isExhausted, "isExhausted", "()Z") \
 METHOD (setPosition, "setPosition", "(J)Z") \

DECLARE_JNI_CLASS (HTTPStream, JUCE_ANDROID_APP_CLASSPATH "$HTTPStream");
#undef JNI_CLASS_MEMBERS


//==============================================================================
void MACAddress::findAllAddresses (Array<MACAddress>& result)
{
    // TODO
}


JUCE_API bool JUCE_CALLTYPE Process::openEmailWithAttachments (const String& targetEmailAddress,
                                                               const String& emailSubject,
                                                               const String& bodyText,
                                                               const StringArray& filesToAttach)
{
    // TODO
    return false;
}

/* Pimpl (String address, bool isPost, const MemoryBlock& postData,
 URL::OpenStreamProgressCallback* progressCallback, void* progressCallbackContext,
 const String& headers, int timeOutMs, StringPairArray* responseHeaders,
 const int numRedirectsToFollow, const String& httpRequest)
 : statusCode (0)

*/
//==============================================================================
class WebInputStream::Pimpl
{
public:
    Pimpl (WebInputStream& pimplOwner, const URL& urlToCopy, bool shouldBePost)
        : statusCode (0), owner (pimplOwner), url (urlToCopy), isPost (shouldBePost),
          numRedirectsToFollow (5), timeOutMs (0), httpRequest (isPost ? "POST" : "GET")
    {}

    ~Pimpl()
    {
        cancel();
    }

    void cancel()
    {
        if (stream != 0)
        {
            stream.callVoidMethod (HTTPStream.release);
            stream.clear();
        }
    }

    bool connect (WebInputStream::Listener* listener)
    {
        String address = url.toString (! isPost);

        if (! address.contains ("://"))
            address = "http://" + address;

        MemoryBlock postData;
        if (isPost)
            WebInputStream::createHeadersAndPostData (url, headers, postData);

        JNIEnv* env = getEnv();

        jbyteArray postDataArray = 0;

        if (postData.getSize() > 0)
        {
            postDataArray = env->NewByteArray (postData.getSize());
            env->SetByteArrayRegion (postDataArray, 0, postData.getSize(), (const jbyte*) postData.getData());
        }

        LocalRef<jobject> responseHeaderBuffer (env->NewObject (StringBuffer, StringBuffer.constructor));

        // Annoyingly, the android HTTP functions will choke on this call if you try to do it on the message
        // thread. You'll need to move your networking code to a background thread to keep it happy..
        jassert (Thread::getCurrentThread() != nullptr);

        jintArray statusCodeArray = env->NewIntArray (1);
        jassert (statusCodeArray != 0);

        stream = GlobalRef (env->CallStaticObjectMethod (JuceAppActivity,
                                                         JuceApp.createHTTPStream,
                                                         javaString (address).get(),
                                                         (jboolean) isPost,
                                                         postDataArray,
                                                         javaString (headers).get(),
                                                         (jint) timeOutMs,
                                                         statusCodeArray,
                                                         responseHeaderBuffer.get(),
                                                         (jint) numRedirectsToFollow,
                                                         javaString (httpRequest).get()));

        jint* const statusCodeElements = env->GetIntArrayElements (statusCodeArray, 0);
        statusCode = statusCodeElements[0];
        env->ReleaseIntArrayElements (statusCodeArray, statusCodeElements, 0);
        env->DeleteLocalRef (statusCodeArray);

        if (postDataArray != 0)
            env->DeleteLocalRef (postDataArray);

        if (stream != 0)
        {
            StringArray headerLines;

            {
                LocalRef<jstring> headersString ((jstring) env->CallObjectMethod (responseHeaderBuffer.get(),
                                                                                  StringBuffer.toString));
                headerLines.addLines (juceString (env, headersString));
            }

            for (int i = 0; i < headerLines.size(); ++i)
            {
                const String& header = headerLines[i];
                const String key (header.upToFirstOccurrenceOf (": ", false, false));
                const String value (header.fromFirstOccurrenceOf (": ", false, false));
                const String previousValue (responseHeaders[key]);

                responseHeaders.set (key, previousValue.isEmpty() ? value : (previousValue + "," + value));
            }

            return true;
        }

        return false;
    }

    //==============================================================================
    // WebInputStream methods
    void withExtraHeaders (const String& extraHeaders)
    {
        if (! headers.endsWithChar ('\n') && headers.isNotEmpty())
            headers << "\r\n";

        headers << extraHeaders;

        if (! headers.endsWithChar ('\n') && headers.isNotEmpty())
            headers << "\r\n";
    }

    void withCustomRequestCommand (const String& customRequestCommand)    { httpRequest = customRequestCommand; }
    void withConnectionTimeout (int timeoutInMs)                          { timeOutMs = timeoutInMs; }
    void withNumRedirectsToFollow (int maxRedirectsToFollow)              { numRedirectsToFollow = maxRedirectsToFollow; }
    StringPairArray getRequestHeaders() const                             { return WebInputStream::parseHttpHeaders (headers); }
    StringPairArray getResponseHeaders() const                            { return responseHeaders; }
    int getStatusCode() const                                             { return statusCode; }

    //==============================================================================
    bool isError() const                         { return stream == nullptr; }

    bool isExhausted()                           { return stream != nullptr && stream.callBooleanMethod (HTTPStream.isExhausted); }
    int64 getTotalLength()                       { return stream != nullptr ? stream.callLongMethod (HTTPStream.getTotalLength) : 0; }
    int64 getPosition()                          { return stream != nullptr ? stream.callLongMethod (HTTPStream.getPosition) : 0; }
    bool setPosition (int64 wantedPos)           { return stream != nullptr && stream.callBooleanMethod (HTTPStream.setPosition, (jlong) wantedPos); }

    int read (void* buffer, int bytesToRead)
    {
        jassert (buffer != nullptr && bytesToRead >= 0);

        if (stream == nullptr)
            return 0;

        JNIEnv* env = getEnv();

        jbyteArray javaArray = env->NewByteArray (bytesToRead);

        int numBytes = stream.callIntMethod (HTTPStream.read, javaArray, (jint) bytesToRead);

        if (numBytes > 0)
            env->GetByteArrayRegion (javaArray, 0, numBytes, static_cast<jbyte*> (buffer));

        env->DeleteLocalRef (javaArray);
        return numBytes;
    }

    //==============================================================================
    int statusCode;

private:
    WebInputStream& owner;
    const URL url;
    bool isPost;
    int numRedirectsToFollow, timeOutMs;
    String httpRequest, headers;
    StringPairArray responseHeaders;

    GlobalRef stream;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
};

URL::DownloadTask* URL::downloadToFile (const File& targetLocation, String extraHeaders, DownloadTask::Listener* listener)
{
    return URL::DownloadTask::createFallbackDownloader (*this, targetLocation, extraHeaders, listener);
}
