#include "jni.h"
#include "jni_bind_release.h"

#include <cstring>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <string>

static const jint JNI_VERSION = JNI_VERSION_10;

static constexpr jni::Class jclassThrowable{
    "java/lang/Throwable",
    jni::Method{"getMessage", jni::Return{jstring{}}},
};

static constexpr jni::Class jclassDigestType{
    "org/apache/bookkeeper/client/BookKeeper$DigestType",
    jni::Static{
        jni::Field{"DUMMY", jni::Self{}},
    },
};

static constexpr jni::Class jclassAbstractConfiguration{
    "org/apache/bookkeeper/conf/AbstractConfiguration",
};

static constexpr jni::Class jclassClientConfiguration{
    "org/apache/bookkeeper/conf/ClientConfiguration",
    jni::Constructor{},
    jni::Method{"setMetadataServiceUri", jni::Return{jclassAbstractConfiguration}, jni::Params{jstring{}}},
};

static constexpr jni::Class jclassLedgerHandle{
    "org/apache/bookkeeper/client/LedgerHandle",
    jni::Method{"append", jni::Return{jlong{}}, jni::Params{jni::Array{jbyte{}}}},
    jni::Method{"close", jni::Return{}},
};

static constexpr jni::Class jclassBookKeeper{
    "org/apache/bookkeeper/client/BookKeeper",
    jni::Constructor{jclassClientConfiguration},
    jni::Method{"createLedger", jni::Return{jclassLedgerHandle}, jni::Params{jint{}, jint{}, jclassDigestType, jni::Array{jbyte{}}}},
    jni::Method{"close", jni::Return{}},
};

void check_java_exception(JNIEnv *env) {
  jobject jvm_throwable = env->ExceptionOccurred();
  if (jvm_throwable != nullptr) {
    env->ExceptionClear();
    jni::LocalObject<jclassThrowable> throwable{jvm_throwable};
    jni::LocalString message = throwable("getMessage");
    jni::UtfStringView message_pinned = message.Pin();
    std::string_view message_view = message_pinned.ToString();
    throw std::runtime_error(message_view.data());
  }
}

class LedgerHandle {
  friend class BookKeeper;

  jni::GlobalObject<jclassLedgerHandle> instance;

  LedgerHandle(jni::GlobalObject<jclassLedgerHandle> instance) : instance{std::move(instance)} {}

public:
  ~LedgerHandle() {
    try {
      close();
    } catch (std::exception &e) {
      std::cerr << "Ledger close error: " << e.what() << std::endl;
    }
  }

  long append(const char *const payload_content, int payload_length) {
    jni::LocalArray<jbyte> payload{payload_length};
    {
      jni::ArrayView<jbyte> pinned = payload.Pin();
      memcpy(pinned.ptr(), payload_content, payload_length);
    }
    jlong entry_id = instance("append", payload);
    check_java_exception(jni::JniEnv::GetEnv());
    return entry_id;
  }

  void close() {
    instance("close");
    check_java_exception(jni::JniEnv::GetEnv());
  }
};

class BookKeeper {
  jni::GlobalObject<jclassBookKeeper> instance;

public:
  BookKeeper(jni::GlobalObject<jclassClientConfiguration> &client_configuration)
      : instance{jni::GlobalObject<jclassBookKeeper>{client_configuration}} {
    check_java_exception(jni::JniEnv::GetEnv());
  }

  ~BookKeeper() {
    try {
      close();
    } catch (std::exception &e) {
      std::cerr << "BookKeeper close error: " << e.what() << std::endl;
    }
  }

  LedgerHandle createLedger(int ensemble_size, int quorum_size, jni::GlobalObject<jclassDigestType> &digest_type, const char *const digest_password_content, int digest_password_length) {
    jni::LocalArray<jbyte> digest_password{digest_password_length};
    {
      jni::ArrayView<jbyte> pinned = digest_password.Pin();
      memcpy(pinned.ptr(), digest_password_content, digest_password_length);
    }
    jni::GlobalObject<jclassLedgerHandle> ledger_handle = instance("createLedger", jint{ensemble_size}, jint{quorum_size}, digest_type, digest_password);
    check_java_exception(jni::JniEnv::GetEnv());
    return LedgerHandle(std::move(ledger_handle));
  }

  void close() {
    instance("close");
    check_java_exception(jni::JniEnv::GetEnv());
  }
};

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

void run(JavaVM *jvm) {
  jni::JvmRef<jni::kDefaultJvm> jvm_ref = jni::JvmRef<jni::kDefaultJvm>(jvm);

  jni::GlobalObject<jclassClientConfiguration> client_configuration{};
  client_configuration("setMetadataServiceUri", "zk+hierarchical://localhost:2181/ledgers");

  BookKeeper bookkeeper_client = BookKeeper(client_configuration);

  jni::GlobalObject<jclassDigestType> digest_type = jni::StaticRef<jclassDigestType>{}["DUMMY"].Get();
  LedgerHandle ledger_handle = bookkeeper_client.createLedger(3, 3, digest_type, "", 0);

  std::string payload_content = "hello";
  for (int i = 0; i < 100; i++) {
    try {
      jlong entry_id = ledger_handle.append(payload_content.c_str(), payload_content.length());
      std::cout << "Appended id=" << entry_id << std::endl;
    } catch (std::exception &e) {
      std::cerr << "Append error: " << e.what() << std::endl;
    }
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

  run(jvm);
  return 0;
}
