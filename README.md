# How-tos

`libjvm` is dynamically link so remember to set `LD_LIBRARY_PATH=$JAVA_HOME/lib:$JAVA_HOME/lib/server`

Do the following steps to get it working (only Linux is supported):
1. Create two new directories in the project's root - `build` and `vendor`
2. Run `mvn prepare-package` to get all Java dependencies into `vendor`
3. Change directory into `build`
4. Run `cmake -DJAVA_HOME=<path to JAVA_HOME> ..` and `make` to build the executable
5. Change directory to the project's root (classpath is hardcoded)
5. Run the executable `./build/bookkeeper-client-jni`

# References

+ JNI specification:
    + https://docs.oracle.com/en/java/javase/17/docs/specs/jni/
+ Managing object life-cycle:
    + https://stackoverflow.com/questions/9098890/how-to-check-for-memory-leaks-in-jni
    + https://stackoverflow.com/questions/42310107/what-are-the-rules-for-jni-local-references-returned-to-a-program-that-embeds-a
