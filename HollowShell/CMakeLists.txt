add_executable (hollow_shell 
	"HollowShell.cpp" 
	
	"grammar/HollowLexer.cpp" 
	"grammar/HollowLexer.h"
	"grammar/HollowParser.cpp" 
	"grammar/HollowParser.h" 
	"grammar/HollowAST.h"
	
	"exec/HollowExecutor.cpp" 
	"exec/HollowExecutor.h"
	"exec/HollowBuiltins.cpp"
	"exec/HollowBuiltins.h"
	"exec/HollowEnvironment.cpp"
	"exec/HollowEnvironment.h" 
	"exec/HollowJob.cpp"
	"exec/HollowJob.h"

	"feature/HollowPrompt.cpp" 
	"feature/HollowPrompt.h" 
	
	"utils/HandlingExecute.h"
	"utils/IgnoreSignal.cpp"
	"utils/IgnoreSignal.h"
)

target_compile_features(hollow_shell PRIVATE cxx_std_20)
target_compile_options(hollow_shell PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)