cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "EAThread" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Base"
	"source/eathread.cpp"
	"source/eathread_barrier.cpp"
	"source/eathread_callstack.cpp"
	"source/eathread_condition.cpp"
	"source/eathread_futex.cpp"
	"source/eathread_mutex.cpp"
	"source/eathread_pool.cpp"
	"source/eathread_rwmutex.cpp"
	"source/eathread_rwmutex_ip.cpp"
	"source/eathread_semaphore.cpp"
	"source/eathread_storage.cpp"
	"source/eathread_thread.cpp"
	"source/version.cpp"
)

add_sources( SOURCE_GROUP "Base/Include"
	"include/eathread/eathread.h"
	"include/eathread/eathread_atomic.h"
	"include/eathread/eathread_barrier.h"
	"include/eathread/eathread_callstack.h"
	"include/eathread/eathread_callstack_context.h"
	"include/eathread/eathread_condition.h"
	"include/eathread/eathread_futex.h"
	"include/eathread/eathread_list.h"
	"include/eathread/eathread_mutex.h"
	"include/eathread/eathread_pool.h"
	"include/eathread/eathread_rwmutex.h"
	"include/eathread/eathread_rwmutex_ip.h"
	"include/eathread/eathread_rwsemalock.h"
	"include/eathread/eathread_rwspinlock.h"
	"include/eathread/eathread_rwspinlockw.h"
	"include/eathread/eathread_semaphore.h"
	"include/eathread/eathread_spinlock.h"
	"include/eathread/eathread_storage.h"
	"include/eathread/eathread_sync.h"
	"include/eathread/eathread_thread.h"
	"include/eathread/shared_array_mt.h"
	"include/eathread/shared_ptr_mt.h"
	"include/eathread/version.h"
)

add_sources( SOURCE_GROUP "Base/Include/Internal"
	"include/eathread/internal/config.h"
	"include/eathread/internal/dllinfo.h"
	"include/eathread/internal/eathread_atomic_standalone.h"
	"include/eathread/internal/eathread_atomic_standalone_gcc.h"
	"include/eathread/internal/eathread_atomic_standalone_msvc.h"
	"include/eathread/internal/eathread_global.h"
	"include/eathread/internal/timings.h"
)

#add_sources( SOURCE_GROUP "PC64"
#	"source/pc/eathread_callstack_win64.cpp"
#	"source/pc/eathread_mutex_pc.cpp"
#	"source/pc/eathread_pc.cpp"
#	"source/pc/eathread_semaphore_pc.cpp"
#	"source/pc/eathread_thread_pc.cpp"
#)

#add_sources( SOURCE_GROUP "PC64/Include"
#	"include/eathread/x86-64/eathread_atomic_x86-64.h"
#	"include/eathread/x86-64/eathread_sync_x86-64.h"
#)

# add_sources( SOURCE_GROUP "CPP11"
# 	"source/cpp11/eathread_cpp11.cpp"
# 	"source/cpp11/eathread_mutex_cpp11.cpp"
# 	"source/cpp11/eathread_semaphore_cpp11.cpp"
# 	"source/cpp11/eathread_thread_cpp11.cpp"
# )
# 
# add_sources( SOURCE_GROUP "CPP11/Include"
# 	"include/eathread/cpp11/eathread_atomic_cpp11.h"
# )

end_sources()
thirdparty_suppress_warnings()

target_include_directories( ${PROJECT_NAME} PRIVATE
	"${THIRDPARTY_SOURCE_DIR}/ea/"
	"${THIRDPARTY_SOURCE_DIR}/ea/EAThread/include/"
)
