#include <cstring>
#include <filesystem>
#include <iostream>
#include <jni.h>
#include <stdio.h>
#include <string>

static const jint JNI_VERSION = JNI_VERSION_10;

namespace java {

namespace lang {

static jclass jc_Throwable;
static jmethodID jm_Throwable_getMessage;

} // namespace lang

} // namespace java

namespace bookkeeper {

namespace conf {

static jclass jc_ClientConfiguration;
static jmethodID jm_ClientConfiguration_init;
static jmethodID jm_ClientConfiguration_setMetadataServiceUri;

} // namespace conf

namespace client {

static jclass jc_BookKeeper;
static jmethodID jm_BookKeeper_init;
static jmethodID jm_BookKeeper_createLedger;
static jmethodID jm_BookKeeper_close;

static jclass jc_BookKeeper_DigestType;
static jfieldID jf_BookKeeper_DigestType_DUMMY;

static jclass jc_LedgerHandle;
static jmethodID jm_LedgerHandle_append;
static jmethodID jm_LedgerHandle_close;

} // namespace client

} // namespace bookkeeper

jclass mustFindClass(JNIEnv *env, const std::string &name) {
  jclass clazz = env->FindClass(name.c_str());
  if (clazz == nullptr) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    exit(1);
  }
  return clazz;
}

jfieldID mustGetStaticFieldID(JNIEnv *env, const jclass clazz, const std::string &name, const std::string &signature) {
  jfieldID id = env->GetStaticFieldID(clazz, name.c_str(), signature.c_str());
  if (id == nullptr) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    exit(1);
  }
  return id;
}

jmethodID mustGetMethodID(JNIEnv *env, const jclass clazz, const std::string &name, const std::string &signature) {
  jmethodID id = env->GetMethodID(clazz, name.c_str(), signature.c_str());
  if (id == nullptr) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    exit(1);
  }
  return id;
}

void loadClassAndMethodReferences(JNIEnv *env) {
  // Load and cache java.lang.Throwable.
  {
    jclass clazz = mustFindClass(env, "java/lang/Throwable");
    java::lang::jc_Throwable = (jclass)env->NewGlobalRef(clazz);
    java::lang::jm_Throwable_getMessage = mustGetMethodID(env, clazz, "getMessage", "()Ljava/lang/String;");
    env->DeleteLocalRef(clazz);
  }
  // Load and cache org.apache.bookkeeper.conf.ClientConfiguration.
  {
    jclass clazz = mustFindClass(env, "org/apache/bookkeeper/conf/ClientConfiguration");
    bookkeeper::conf::jc_ClientConfiguration = (jclass)env->NewGlobalRef(clazz);
    bookkeeper::conf::jm_ClientConfiguration_init = mustGetMethodID(env, clazz, "<init>", "()V");
    bookkeeper::conf::jm_ClientConfiguration_setMetadataServiceUri = mustGetMethodID(env, clazz, "setMetadataServiceUri", "(Ljava/lang/String;)Lorg/apache/bookkeeper/conf/AbstractConfiguration;");
    env->DeleteLocalRef(clazz);
  }
  // Load and cache org.apache.bookkeeper.client.BookKeeper.
  {
    jclass clazz = mustFindClass(env, "org/apache/bookkeeper/client/BookKeeper");
    bookkeeper::client::jc_BookKeeper = (jclass)env->NewGlobalRef(clazz);
    bookkeeper::client::jm_BookKeeper_init = mustGetMethodID(env, clazz, "<init>", "(Lorg/apache/bookkeeper/conf/ClientConfiguration;)V");
    bookkeeper::client::jm_BookKeeper_createLedger = mustGetMethodID(env, clazz, "createLedger", "(IILorg/apache/bookkeeper/client/BookKeeper$DigestType;[B)Lorg/apache/bookkeeper/client/LedgerHandle;");
    bookkeeper::client::jm_BookKeeper_close = mustGetMethodID(env, clazz, "close", "()V");
    env->DeleteLocalRef(clazz);
  }
  // Load and cache org.apache.bookkeeper.client.BookKeeper.DigestType.
  {
    jclass clazz = mustFindClass(env, "org/apache/bookkeeper/client/BookKeeper$DigestType");
    bookkeeper::client::jc_BookKeeper_DigestType = (jclass)env->NewGlobalRef(clazz);
    bookkeeper::client::jf_BookKeeper_DigestType_DUMMY = mustGetStaticFieldID(env, clazz, "DUMMY", "Lorg/apache/bookkeeper/client/BookKeeper$DigestType;");
    env->DeleteLocalRef(clazz);
  }
  // Load and cache org.apache.bookkeeper.client.LedgerHandle.
  {
    jclass clazz = mustFindClass(env, "org/apache/bookkeeper/client/LedgerHandle");
    bookkeeper::client::jc_LedgerHandle = (jclass)env->NewGlobalRef(clazz);
    bookkeeper::client::jm_LedgerHandle_append = mustGetMethodID(env, clazz, "append", "([B)J");
    bookkeeper::client::jm_LedgerHandle_close = mustGetMethodID(env, clazz, "close", "()V");
    env->DeleteLocalRef(clazz);
  }
}

void unloadClassAndMethodReferences(JNIEnv *env) {
  env->DeleteGlobalRef(java::lang::jc_Throwable);
  env->DeleteGlobalRef(bookkeeper::conf::jc_ClientConfiguration);
  env->DeleteGlobalRef(bookkeeper::client::jc_BookKeeper);
  env->DeleteGlobalRef(bookkeeper::client::jc_BookKeeper_DigestType);
  env->DeleteGlobalRef(bookkeeper::client::jc_LedgerHandle);
}

std::string readJarPaths(std::string directory) {
  std::string jar_paths = "";
  try {
    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
      if (entry.is_regular_file() && entry.path().extension() == ".jar") {
        if (!jar_paths.empty()) {
          jar_paths.append(":");
        }
        jar_paths.append(entry.path().string());
      }
    }
  } catch (const std::filesystem::filesystem_error &e) {
    std::cerr << "Filesystem error: " << e.what() << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "General error: " << e.what() << std::endl;
  }
  return jar_paths;
}

void run(JNIEnv *env) {
  jobject obj_bookkeeper_client_config = env->NewObject(bookkeeper::conf::jc_ClientConfiguration, bookkeeper::conf::jm_ClientConfiguration_init);
  jstring zookeeper_address = env->NewStringUTF("zk+hierarchical://localhost:2181/ledgers");
  env->CallNonvirtualObjectMethod(obj_bookkeeper_client_config, bookkeeper::conf::jc_ClientConfiguration, bookkeeper::conf::jm_ClientConfiguration_setMetadataServiceUri, zookeeper_address);
  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    return;
  }
  jobject obj_bookkeeper_client = env->NewObject(bookkeeper::client::jc_BookKeeper, bookkeeper::client::jm_BookKeeper_init, obj_bookkeeper_client_config);
  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    return;
  }
  jobject digest_type = env->GetStaticObjectField(bookkeeper::client::jc_BookKeeper_DigestType, bookkeeper::client::jf_BookKeeper_DigestType_DUMMY);
  jbyteArray digest_password = env->NewByteArray(0);
  jobject obj_ledger_handle = env->CallNonvirtualObjectMethod(obj_bookkeeper_client, bookkeeper::client::jc_BookKeeper, bookkeeper::client::jm_BookKeeper_createLedger, 3, 3, digest_type, digest_password);
  if (!env->ExceptionCheck()) {
    std::string payload_content = "hello";
    for (int i = 0; i < 100; i++) {
      jbyteArray payload = env->NewByteArray(5);
      jbyte *payload_buffer = env->GetByteArrayElements(payload, nullptr);
      memcpy(payload_buffer, payload_content.c_str(), 5);
      env->ReleaseByteArrayElements(payload, payload_buffer, JNI_ABORT);
      jlong entry_id = env->CallNonvirtualLongMethod(obj_ledger_handle, bookkeeper::client::jc_LedgerHandle, bookkeeper::client::jm_LedgerHandle_append, payload);
      if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
      } else {
        printf("Append entry with id=%ld\n", entry_id);
      }
      env->DeleteLocalRef(payload);
    }
    env->CallNonvirtualVoidMethod(obj_ledger_handle, bookkeeper::client::jc_LedgerHandle, bookkeeper::client::jm_LedgerHandle_close);
    if (env->ExceptionCheck()) {
      env->ExceptionDescribe();
      env->ExceptionClear();
    }
  } else {
    env->ExceptionDescribe();
    env->ExceptionClear();
  }
  env->CallNonvirtualVoidMethod(obj_ledger_handle, bookkeeper::client::jc_LedgerHandle, bookkeeper::client::jm_LedgerHandle_close);
  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
  }
  env->CallNonvirtualVoidMethod(obj_bookkeeper_client, bookkeeper::client::jc_BookKeeper, bookkeeper::client::jm_BookKeeper_close);
  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
  }
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  std::string classpath = "-Djava.class.path=";
  classpath.append(readJarPaths("vendor"));
  JavaVMOption options[1];
  options[0].optionString = classpath.data();
  JavaVMInitArgs init_args = {
      .version = JNI_VERSION,
      .nOptions = 1,
      .options = options,
      .ignoreUnrecognized = false,
  };
  JavaVM *jvm;
  JNIEnv *env;
  if (JNI_CreateJavaVM(&jvm, (void **)&env, (void *)&init_args) != JNI_OK) {
    fprintf(stderr, "Failed to create the JVM\n");
    return 1;
  }
  loadClassAndMethodReferences(env);
  run(env);
  unloadClassAndMethodReferences(env);
  jvm->DestroyJavaVM();
  return 0;
}
