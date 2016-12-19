#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "jvmti.h"
#include <mpi.h>
#include <utmpx.h>
#include <pthread.h>
#include <time.h>

typedef struct {
  /* JVMTI Environment */
  jvmtiEnv      *jvmti;
  jboolean       vm_is_started;
  /* Data access Lock */
  jrawMonitorID  lock;
} GlobalAgentData;

typedef jlong (JNICALL *call__jlong) (JNIEnv*, jclass);

static jvmtiEnv *jvmti = NULL;
static GlobalAgentData *gdata;
static call__jlong originalMethodCurrentTimeInMillis = NULL;

pthread_mutex_t lock;
pthread_mutex_t bindlock;
int try_cnt = 0;
int suc_cnt = 0;


// static 



JNIEXPORT jlong JNICALL Java_System_currentTimeMillis(JNIEnv* env, jclass jc) {


  // pthread_mutex_lock(&lock);
// JNINativeMethod methods[] = {
//     {"currentTimeMillis",    "()J",                    (void *)&Java_System_currentTimeMillis}
// };
//   (*env)->RegisterNatives(env, jc,
//                             methods, sizeof(methods)/sizeof(methods[0]));

  struct timeval t;
  gettimeofday(&t, NULL);
  // printf("%ld --- FAIL: NOT STARTED\n", ((t.tv_sec * 1000000 + t.tv_usec)));
  // printf("Time: %ld%06ld\n", (long int)t.tv_sec, (long int)t.tv_usec);
  // printf("TRY %d times\n", ++try_cnt);

  time_t rawtime;
  struct tm * timeinfo;

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  // printf ( "Current local time and date: %s", asctime (timeinfo) );



  // jclass systemClass = (*env)->FindClass(env, "java/lang/System");
  // jmethodID getPropertyMethodId = (*env)->GetStaticMethodID(env, systemClass, "getProperty", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");

  if (!gdata->vm_is_started) {
    // printf("%ld --- FAIL: NOT STARTED\n", ((t.tv_sec * 1000000 + t.tv_usec)));
    // pthread_mutex_unlock(&lock);

    return originalMethodCurrentTimeInMillis(env, jc);
  }

  // jstring offsetPropertyName = (*env)->NewStringUTF(env, "faketime.millis");
  // jstring offsetPropertyDefault = (*env)->NewStringUTF(env, "0");

  // jstring offsetValue = (*env)->CallStaticObjectMethod(env, systemClass, getPropertyMethodId, offsetPropertyName, offsetPropertyDefault);
  if ((*env)->ExceptionCheck(env)) {
    printf("FAIL: EXCEPTION\n");
    // pthread_mutex_unlock(&lock);
    return 0;
  }

  // const char *offset = (*env)->GetStringUTFChars(env, offsetValue, NULL);
// jlong realTime = originalMethodCurrentTimeInMillis(env, jc);
  jlong timeWithOffset = 0;//atol(offset);

  // printf("Returning value with offset for ");
  // MPI_Init(&argc, &argv);
  // printf( "cpu = %d\n", sched_getcpu() );
  // MPI_Finalize();
  // printf("SUCCESS %d times\n", ++suc_cnt);

  // (*env)->ReleaseStringUTFChars(env, offsetValue, offset);

  // pthread_mutex_unlock(&lock);

  return timeWithOffset;
}

static void check_jvmti_error(jvmtiEnv *jvmti_env, jvmtiError errnum, const char *str)
{
  if (errnum != JVMTI_ERROR_NONE) {
    char *errnum_str;

    errnum_str = NULL;
    (void)(*jvmti_env)->GetErrorName(jvmti_env, errnum, &errnum_str);

    printf("ERROR: JVMTI: %d(%s): %s\n", errnum, (errnum_str==NULL?"Unknown":errnum_str), (str==NULL?"":str));
  }
}

/* Enter a critical section by doing a JVMTI Raw Monitor Enter */
static void enter_critical_section(jvmtiEnv *jvmti_env)
{
  jvmtiError error;

  error = (*jvmti_env)->RawMonitorEnter(jvmti_env, gdata->lock);
  check_jvmti_error(jvmti_env, error, "Cannot enter with raw monitor");
}

/* Exit a critical section by doing a JVMTI Raw Monitor Exit */
static void exit_critical_section(jvmtiEnv *jvmti_env)
{
  jvmtiError error;

  error = (*jvmti_env)->RawMonitorExit(jvmti_env, gdata->lock);
  check_jvmti_error(jvmti_env, error, "Cannot exit with raw monitor");
}

void JNICALL callbackNativeMethodBind(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread, jmethodID method, void* address, void** new_address_ptr) {
    // printf("TRYBIND\n");
  pthread_mutex_lock(&bindlock);
  enter_critical_section(jvmti_env);



   {
    char *methodName = NULL;
    char *declaringClassName = NULL;
    jclass declaring_class;
    jvmtiError err;




    // *jvmti = jvmti_env; 



    char* methodSignature = NULL;
    char* methodGenericPtr = NULL;
    err = (*jvmti_env)->GetMethodName(jvmti_env, method, &methodName, &methodSignature, &methodGenericPtr);

    // char* p = &(methodName[0]);
    // for ( ; *p; ++p) *p = tolower(*p);
    // if (strstr(methodName, "milli")) {
    //   printf("MILLIZ-----\n\n\n");
    // }

    // methodName=&(methodName[0]);

    // printf("Trying binding for %s\n", methodName);


    if (err == JVMTI_ERROR_NONE && strcmp("currentTimeMillis", methodName) == 0) {
      err = (*jvmti_env)->GetMethodDeclaringClass(jvmti_env, method, &declaring_class);
      if (err != JVMTI_ERROR_NONE) {
          // printf("Error getting declaring class %d\n", err);
      }
      err = (*jvmti_env)->GetClassSignature(jvmti_env, declaring_class, &declaringClassName, NULL);
      // if (strcmp("Ljava/lang/System$2;", declaringClassName) == 0) {
      //     printf("WHATTTTTTT\n\n\n\n");
      // }
      if (err == JVMTI_ERROR_NONE && strcmp("Ljava/lang/System;", declaringClassName) == 0) {
        // if (originalMethodCurrentTimeInMillis == NULL) {
          printf("Did binding\n");
          printf("Name %s, signature: %s, generic ptr: %s", methodName, methodSignature, methodGenericPtr);
          originalMethodCurrentTimeInMillis = address;
          // address = NULL;
          *new_address_ptr = (void *) &Java_System_currentTimeMillis;
          printf("new addr pointing at %p\n", (void *) &Java_System_currentTimeMillis);
        // }
      } //else {
          // printf("Error getting class signature %d\n", err);

      // }
      check_jvmti_error(jvmti_env, (*jvmti_env)->Deallocate(jvmti_env, (unsigned char *)declaringClassName), "Nope");
    } else {
          // printf("Error getting method name %d, resolvedMethodName: %s\n", err, methodName);
    }

    check_jvmti_error(jvmti_env, (*jvmti_env)->Deallocate(jvmti_env, (unsigned char *)methodName), "Err");
    check_jvmti_error(jvmti_env, (*jvmti_env)->Deallocate(jvmti_env, (unsigned char *)methodSignature), "Err");
    check_jvmti_error(jvmti_env, (*jvmti_env)->Deallocate(jvmti_env, (unsigned char *)methodGenericPtr), "Err");
  }
  exit_critical_section(jvmti_env);

  pthread_mutex_unlock(&bindlock);
}

void JNICALL callbackVMInit(jvmtiEnv *jvmti, JNIEnv* jni_env, jthread thread) {
  gdata->vm_is_started = JNI_TRUE;
  check_jvmti_error(jvmti, (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_NATIVE_METHOD_BIND, (jthread)NULL), "Cannot set event notification");

}

void JNICALL
callbackMethodExit(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env,
            jthread thread,
            jmethodID method,
            jboolean was_popped_by_exception,
            jvalue return_value) {

  // char *methodName;
  // jvmtiError err = (*jvmti)->GetMethodName(jvmti, method, &methodName, NULL, NULL);
  // if (err == JVMTI_ERROR_NONE && strcmp("currentTimeMillis", methodName) == 0 && ((long int) return_value.j) != 0) {
  //     printf("Method Name: %s, ret val: %ld\n", methodName, (long int) return_value.j);
  // }

}

void JNICALL
callbackMethodEntry(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env,
            jthread thread,
            jmethodID method) {


    // GetMethodLocation(jvmtiEnv* env,
    //         jmethodID method,
    //         jlocation* start_location_ptr,
    //         jlocation* end_location_ptr)


    char* methodName = NULL;
    jvmtiError err = (*jvmti_env)->GetMethodName(jvmti_env, method, &methodName, NULL, NULL);

     if (err == JVMTI_ERROR_NONE && strcmp("currentTimeMillis", methodName) == 0) {
        jlocation start, end;
        err = (*jvmti_env)->GetMethodLocation(jvmti_env, method, &start, &end);
        printf("Location start: %p, end: %p\n", start, end);
     }


  // TODO: if method is not CTM, disable for current jthread

  // char *methodName;
  // jvmtiError err = (*jvmti)->GetMethodName(jvmti, method, &methodName, NULL, NULL);
  // if (err == JVMTI_ERROR_NONE && strcmp("currentTimeMillis", methodName) == 0) {
  //   // check_jvmti_error(jvmti, (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_DISABLE, JVMTI_EVENT_METHOD_ENTRY, NULL), "Cannot set evt notif");

  //     // printf("Method Name: %s in thread %d\n", methodName, thread);
  // } else {
  //   // check_jvmti_error(jvmti, (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_DISABLE, JVMTI_EVENT_METHOD_ENTRY, thread), "Cannot set evt notif");
  // }

}


void JNICALL
callbackDynamicCodeGenerated(jvmtiEnv *jvmti_env,
            const char* name,
            const void* address,
            jint length) 

{
  printf("Dynamic code generated for %s\n", name);
}



void JNICALL
callbackCompiledMethodUnload(jvmtiEnv *jvmti_env,
            jmethodID method,
            const void* code_addr) 

{
  printf("Compiled method unload");
}

void JNICALL
callbackCompiledMethodLoad(jvmtiEnv *jvmti,
            jmethodID method,
            jint code_size,
            const void* code_addr,
            jint map_length,
            const jvmtiAddrLocationMap* map,
            const void* compile_info) 

{
  char *methodName;
  jvmtiError err = (*jvmti)->GetMethodName(jvmti, method, &methodName, NULL, NULL);
  printf("Compiled method LOAD for method %s\n", methodName);

}


void JNICALL
callbackClassFileLoadHook(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env,
            jclass class_being_redefined,
            jobject loader,
            const char* name,
            jobject protection_domain,
            jint class_data_len,
            const unsigned char* class_data,
            jint* new_class_data_len,
            unsigned char** new_class_data) 

{

  int len = 17;
  char* fn = "currentTimeMillis";

  if (strcmp("java/lang/System", name) == 0) {

        *new_class_data_len = class_data_len + 2;
        jvmtiError err = (*jvmti)->Allocate(jvmti, (jlong) *new_class_data_len, new_class_data);

        // printf("old len: %d, new len: %d\n", class_data_len, *new_class_data_len);
        


        int k;
        // for (k=0; k < *new_class_data_len; k++) {
        //   printf("at %d, val: %c\n", k, (*new_class_data)[k]);

        // }

        // printf("Loading class %s\n", name);
        // printf("Class len: %d\n", class_data_len);



        int i;
        int new_data_idx = 0;
        for (i = 0; i < class_data_len; i++) {
          unsigned char c = class_data[i];
          if (c > 31 && c < 127) {
            printf("%c", class_data[i]);
          }

          if (c == fn[0]) {
            int found = 1;
            int j = 1;
            for (; j < len; j++) {
              // printf("data: %c, fn: %c\n", class_data[i+j], fn[j]);
              if ((i + j) >= class_data_len || (class_data[i+j]) != fn[j]) {
                found = 0;
                break;
              }
            }

            if (found) {
              (*new_class_data)[new_data_idx++] = (unsigned char)'m';
              (*new_class_data)[new_data_idx++] = (unsigned char)'y';
            }
          }


          (*new_class_data)[new_data_idx++] = class_data[i];
        }

        printf("\nAFTER\n");




        for (k=0; k < *new_class_data_len; k++) {
            unsigned char c = (*new_class_data)[k];
            if (c > 31 && c < 127) {
              printf("%c", c);
            } 
        }

  }






}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved)
{
  static GlobalAgentData data;
  jvmtiEventCallbacks callbacks;

  // printf("LOADING");
  jint res = (*jvm)->GetEnv(jvm, (void **) &jvmti, JVMTI_VERSION_1_0);
  if (res != JNI_OK || jvmti == NULL) {
    fprintf(stderr, "ERROR: Unable to access JVMTI Version 1");
  }

  (void)memset((void*)&data, 0, sizeof(data));
  gdata = &data;
  gdata->jvmti = jvmti;

  jvmtiCapabilities capa;
  (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
  capa.can_get_owned_monitor_info = 1;
  capa.can_generate_native_method_bind_events = 1;
  capa.can_generate_method_exit_events = 1;
  capa.can_generate_method_entry_events = 1;
  capa.can_generate_all_class_hook_events = 1;
  // capa.can_generate_compiled_method_load_events = 1;
  check_jvmti_error(jvmti, (*jvmti)->AddCapabilities(jvmti, &capa), "Unable to get necessary JVMTI capabilities.");

  (void)memset(&callbacks, 0, sizeof(callbacks));
  callbacks.NativeMethodBind = &callbackNativeMethodBind;
  callbacks.VMInit = &callbackVMInit;
  // callbacks.MethodExit = &callbackMethodExit;
  // callbacks.MethodEntry = &callbackMethodEntry;
  // callbacks.ClassFileLoadHook = &callbackClassFileLoadHook;
  // callbacks.DynamicCodeGenerated = &callbackDynamicCodeGenerated;
  // callbacks.CompiledMethodUnload = &callbackCompiledMethodUnload;
  // callbacks.CompiledMethodLoad = &callbackCompiledMethodLoad;
  check_jvmti_error(jvmti, (*jvmti)->SetEventCallbacks(jvmti, &callbacks, (jint)sizeof(callbacks)), "Cannot set jvmti callbacks");

  check_jvmti_error(jvmti, (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_NATIVE_METHOD_BIND, (jthread)NULL), "Cannot set event notification");
  check_jvmti_error(jvmti, (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, (jthread)NULL), "Cannot set event notification");
  // check_jvmti_error(jvmti, (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_DYNAMIC_CODE_GENERATED, (jthread)NULL), "Cannot set evt notif");
  // check_jvmti_error(jvmti, (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_COMPILED_METHOD_UNLOAD, NULL), "Cannot set evt notif");
  // check_jvmti_error(jvmti, (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_COMPILED_METHOD_LOAD, NULL), "Cannot set evt notif");
  check_jvmti_error(jvmti, (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, (jthread)NULL), "Cannot set evt notif");
  // check_jvmti_error(jvmti, (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL), "Cannot set evt notif");
  // check_jvmti_error(jvmti, (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_METHOD_EXIT, (jthread)NULL), "Cannot set evt notif");


  check_jvmti_error(jvmti, (*jvmti)->CreateRawMonitor(jvmti, "agent data", &(gdata->lock)), "Cannot create raw monitor");

  return JNI_OK;
}

// JNIEXPORT jvmtiError JNICALL GetOwnedMonitorInfo(jvmtiEnv* env,
//             jthread thread,
//             jint* owned_monitor_count_ptr,
//             jobject** owned_monitors_ptr) 
// {

//   printf("HERRRRRRR");
//   return JVMTI_ERROR_NONE;
// }

JNIEXPORT jint JNICALL 
Agent_OnAttach(JavaVM* vm, char *options, void *reserved) {



//   // printf("VM attached!");
//     pthread_mutex_lock(&bindlock);

// jrawMonitorID rawMonLock = gdata->lock;
// jvmtiEnv* oldJvmti = jvmti;

//     if ((*oldJvmti)->RawMonitorEnter(oldJvmti, rawMonLock) != JVMTI_ERROR_NONE) {
//       printf("PROBLEM GETTING RAW MON");
//     }




// static GlobalAgentData data;
//     int res = (*vm)->GetEnv(vm, (void **) &jvmti, JVMTI_VERSION_1_0);
//       if (res != JNI_OK || jvmti == NULL) {
//         printf("XXX: ERROR: Unable to access JVMTI Version 1");
//     }

//       (void)memset((void*)&data, 0, sizeof(data));
//       gdata = &data;
//       gdata->jvmti = jvmti;

//         jvmtiCapabilities capa;

//   (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
//   capa.can_get_owned_monitor_info = 1;
//   capa.can_generate_native_method_bind_events = 1;
//   check_jvmti_error(jvmti, (*jvmti)->AddCapabilities(jvmti, &capa), "Unable to get necessary JVMTI capabilities.");

//   jvmtiEventCallbacks callbacks;

//   (void)memset(&callbacks, 0, sizeof(callbacks));
//   callbacks.NativeMethodBind = &callbackNativeMethodBind;
//   callbacks.VMInit = &callbackVMInit;
//   check_jvmti_error(jvmti, (*jvmti)->SetEventCallbacks(jvmti, &callbacks, (jint)sizeof(callbacks)), "Cannot set jvmti callbacks");


//       check_jvmti_error(jvmti, (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_NATIVE_METHOD_BIND, (jthread)NULL), "Cannot set event notification");
//       check_jvmti_error(jvmti, (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, (jthread)NULL), "Cannot set event notification");
//       check_jvmti_error(jvmti, (*jvmti)->CreateRawMonitor(jvmti, "agent data", &(gdata->lock)), "Cannot create raw monitor");





//     if ((*oldJvmti)->RawMonitorExit(oldJvmti, rawMonLock) != JVMTI_ERROR_NONE) {
//       printf("PROBLEM exiting RAW MON");
//     }
//   // jint rc = Agent_OnLoad(vm, options, reserved);
//     pthread_mutex_unlock(&bindlock);
    // return rc;
    return JNI_OK;
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm)
{
  // printf("UNLOADED\n");
          // printf("new method at %p\n", (void *) &Java_System_currentTimeMillis);

}
