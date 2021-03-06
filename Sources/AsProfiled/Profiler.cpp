// profiler.cpp : Implementation of CProfiler

#include "stdafx.h"
#include "profiler.h"
#include "MethodInfo.h"
#include "AttributeInfo.h"
#include "AttributeArgument.h"
#include "ClousureEvaluator.h"
#include "CorDebug.h"
#include <map>

using namespace log4cxx;

CProfiler* _cProfilerGlobalHandler = NULL;
LoggerPtr myMainLogger(Logger::getLogger("main"));


CProfiler::CProfiler() 
{
	// Configure Log4cxx
	BasicConfigurator::configure();
	// Levels hierarchy: TRACE < DEBUG < INFO < WARN < ERROR < FATAL
	myMainLogger->setLevel(Level::toLevel(log4cxx::Level::INFO_INT));
}


// Static function mapper 
// We might need to map functions in order to track them
UINT_PTR CProfiler::FunctionMapper(FunctionID functionId, BOOL *pbHookFunction) {
	
	if (_cProfilerGlobalHandler != NULL) {
		_cProfilerGlobalHandler->MapFunction(functionId);
	}
	return (UINT_PTR) functionId;
}

// Do whatever we want with function ID
void CProfiler::MapFunction(FunctionID functionId) {
		//cout << "map f" << functionId << endl;
}

// Function called by .NET runtime when function is invoked
void CProfiler::FunctionEnter(FunctionID functionID, UINT_PTR clientData, COR_PRF_FRAME_INFO func, COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo) {
	
	
	// Create methodInfo object which has the entry point for reading every metadata related to called method
	CMethodInfo* methodInfo = new CMethodInfo(functionID, _ICorProfilerInfo2, argumentInfo);
	// Buffer for function name
	WCHAR szMethodName[NAME_BUFFER_SIZE];
	// Read method's full qualified name
	
	HRESULT hr = GetFullMethodName(*methodInfo, szMethodName);
	static bool mainOccured = false;
	std::wstring methodName(szMethodName);
	if (methodName.find(L"Main") != std::string::npos)
		mainOccured = true;
	
	if (mainOccured == true)
		LOG4CXX_DEBUG(myMainLogger, szMethodName); 

	PrintMethodInfo(*methodInfo);

	// A pointer that references the metadata for called function 
	mdToken token = mdTokenNil;
	mdMethodDef mdMethod = mdTypeDefNil;
	// Enumerator for function's parameters
	HCORENUM paramsEnum = NULL;
	// Arguments count
	ULONG paramsCount = 0;
	// Param postion in function signature
	ULONG paramPosition = 0;
	// Parameter name lenght
	ULONG paramNameLen = 0;
	// Parameter's name
	LPWSTR paramName = 0;
	// Parameter's type
	DWORD corElementType = 0;
	
	ULONG32 pcTypeArgs = 0;		
	
	this->attributeReader->Initialize(methodInfo->GetMethodToken(), methodInfo->GetMetaDataImport());
	
	CAttributeInfo* attributeInfo =  this->attributeReader->GetAttribute(L"AsContractAttribute", 2);
	
	if (attributeInfo == NULL) {
		LOG4CXX_TRACE(myMainLogger, "attribute not found");
	}else{
		methodInfo->ReadArgumentsValues(argumentInfo);
		CClousureEvaluator* evaluator = new CClousureEvaluator(methodInfo, attributeInfo, _ICorProfilerInfo2, argumentInfo);
		
		functionsMap->insert(pair<FunctionID, CClousureEvaluator*>( functionID, evaluator));
		LOG4CXX_INFO(myMainLogger, szMethodName); 
		LOG4CXX_INFO(myMainLogger, attributeInfo->arguments->at(0)->argumentValue);
		bool result = evaluator->EvalPreCondition();
		if (result == true) {
			LOG4CXX_INFO(myMainLogger, "Precondition:TRUE");
		}else{
			LOG4CXX_WARN(myMainLogger, "Precondition:FALSE");
			exit(-1);
		}
	}
}

void CProfiler::FunctionLeave(FunctionID functionID, UINT_PTR clientData, COR_PRF_FRAME_INFO func, COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange) {
	
	//cout << "leave " << functionID << endl;
	std::map<FunctionID, CClousureEvaluator*>::iterator iter;
	iter = functionsMap->find(functionID);
	if (iter == functionsMap->end()) {
		return;
	}
	CClousureEvaluator* evaluator = iter->second;
	WCHAR szMethodName[NAME_BUFFER_SIZE];
	// Read method's full qualified name
	
	HRESULT hr = GetFullMethodName(*evaluator->GetMethodInfo(), szMethodName);
	LOG4CXX_INFO(myMainLogger, szMethodName);
	LOG4CXX_INFO(myMainLogger, evaluator->GetAttributeInfo()->arguments->at(1)->argumentValue);
	ASSERT(false);
	bool result = evaluator->EvalPostCondition(retvalRange);
	if (result == true) {
		LOG4CXX_INFO(myMainLogger, "Postcondition:TRUE");
	}else{
		LOG4CXX_WARN(myMainLogger, "Postcondition:FALSE");
		exit(-1);
	}	
	
}


HRESULT CProfiler::GetFunctionData(FunctionID functionId) 
{
	return S_OK;
}

HRESULT CProfiler::GetFullMethodName(CMethodInfo& methodInfo, LPWSTR wszMethod) {
	CTypeInfo typeInfo(methodInfo.GetMetaDataImport(), methodInfo.GetTypeToken());

	wstring fullMethodName(typeInfo.GetName());
	fullMethodName =  fullMethodName.append(L".");
	fullMethodName = fullMethodName.append(methodInfo.GetMethodName());
	memcpy(wszMethod, fullMethodName.c_str(), (fullMethodName.size() + 1) * sizeof(WCHAR) );

	return S_OK;
}

void CProfiler::PrintMethodInfo(CMethodInfo& methodInfo) {
	
	CTypeInfo typeInfo(methodInfo.GetMetaDataImport(), methodInfo.GetTypeToken());
	

	ULONG callConv = methodInfo.GetCallingConvention();
	ULONG paramsCount = 0;

	// Get calling convention 
	paramsCount = methodInfo.GetArgumentsCount();
	LOG4CXX_DEBUG(myMainLogger, "# of arguments: " << methodInfo.GetArgumentsCount());
	
	// Get function's return type
	LPWSTR returnType = new WCHAR[NAME_BUFFER_SIZE];
	returnType[0] = '\0';
	CParamParser* paramParser = new CParamParser(*methodInfo.GetMetaDataImport());
	CParam* param = methodInfo.GetReturnValue();
	LOG4CXX_DEBUG(myMainLogger, param->paramTypeAsString);
	

	// Get method's arguments
	std::vector<CParam*>* arguments = methodInfo.GetArguments();
	for (vector<CParam*>::iterator iter = arguments->begin(); iter != arguments->end(); iter++) {
		LOG4CXX_DEBUG(myMainLogger, (*iter)->paramTypeAsString);
		LOG4CXX_DEBUG(myMainLogger, (*iter)->paramName);
	}	
}


void _stdcall FunctionEnterGlobal(FunctionID functionID, UINT_PTR clientData, COR_PRF_FRAME_INFO func, COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo) {
	_cProfilerGlobalHandler->FunctionEnter(functionID, clientData, func, argumentInfo);
	
}

void _stdcall FunctionLeaveGlobal(FunctionID functionID, UINT_PTR clientData, COR_PRF_FRAME_INFO func, COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange) {
	_cProfilerGlobalHandler->FunctionLeave(functionID, clientData, func, retvalRange);
	
}

// restore call stack
void _declspec(naked) FunctionEnterHandler(FunctionID functionID, UINT_PTR clientData, COR_PRF_FRAME_INFO func, COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo) 
{
	__asm 
	{
		push	ebp // create stack frame
		mov		ebp, esp // move current stack pointer to ebp
		pushad
		mov		eax, [ebp+0x14]
		push	eax
		mov		eax, [ebp+0x10]
		push	eax
		mov		eax, [ebp+0x0C]
		push	eax
		mov		eax, [ebp+0x08]
		push	eax
		call	FunctionEnterGlobal
		popad
		pop		ebp
		ret		16
	}
}


void _declspec(naked) FunctionLeaveHandler(FunctionID functionID, UINT_PTR clientData, COR_PRF_FRAME_INFO func, COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange)
{
	__asm 
	{
		push	ebp
		mov		ebp,esp
		pushad
		mov		eax, [ebp+0x14]
		push	eax
		mov		eax, [ebp+0x10]
		push	eax
		mov		eax, [ebp+0x0C]
		push	eax
		mov		eax, [ebp+0x08]
		push	eax
		call	FunctionLeaveGlobal
		popad
		pop		ebp
		ret		16
	}
}

HRESULT CProfiler::SetEventMask()
{
	//COR_PRF_MONITOR_NONE	= 0,
	//COR_PRF_MONITOR_FUNCTION_UNLOADS	= 0x1,
	//COR_PRF_MONITOR_CLASS_LOADS	= 0x2,
	//COR_PRF_MONITOR_MODULE_LOADS	= 0x4,
	//COR_PRF_MONITOR_ASSEMBLY_LOADS	= 0x8,
	//COR_PRF_MONITOR_APPDOMAIN_LOADS	= 0x10,
	//COR_PRF_MONITOR_JIT_COMPILATION	= 0x20,
	//COR_PRF_MONITOR_EXCEPTIONS	= 0x40,
	//COR_PRF_MONITOR_GC	= 0x80,
	//COR_PRF_MONITOR_OBJECT_ALLOCATED	= 0x100,
	//COR_PRF_MONITOR_THREADS	= 0x200,
	//COR_PRF_MONITOR_REMOTING	= 0x400,
	//COR_PRF_MONITOR_CODE_TRANSITIONS	= 0x800,
	//COR_PRF_MONITOR_ENTERLEAVE	= 0x1000,
	//COR_PRF_MONITOR_CCW	= 0x2000,
	//COR_PRF_MONITOR_REMOTING_COOKIE	= 0x4000 | COR_PRF_MONITOR_REMOTING,
	//COR_PRF_MONITOR_REMOTING_ASYNC	= 0x8000 | COR_PRF_MONITOR_REMOTING,
	//COR_PRF_MONITOR_SUSPENDS	= 0x10000,
	//COR_PRF_MONITOR_CACHE_SEARCHES	= 0x20000,
	//COR_PRF_MONITOR_CLR_EXCEPTIONS	= 0x1000000,
	//COR_PRF_MONITOR_ALL	= 0x107ffff,
	//COR_PRF_ENABLE_REJIT	= 0x40000,
	//COR_PRF_ENABLE_INPROC_DEBUGGING	= 0x80000,
	//COR_PRF_ENABLE_JIT_MAPS	= 0x100000,
	//COR_PRF_DISABLE_INLINING	= 0x200000,
	//COR_PRF_DISABLE_OPTIMIZATIONS	= 0x400000,
	//COR_PRF_ENABLE_OBJECT_ALLOCATED	= 0x800000,
	
	// New in VS2005
	//COR_PRF_ENABLE_FUNCTION_ARGS	= 0x2000000,
	//COR_PRF_ENABLE_FUNCTION_RETVAL	= 0x4000000,
	//COR_PRF_ENABLE_FRAME_INFO	= 0x8000000,
	//COR_PRF_ENABLE_STACK_SNAPSHOT	= 0x10000000,
	//COR_PRF_USE_PROFILE_IMAGES	= 0x20000000,
	// End New in VS2005
	
	//COR_PRF_ALL	= 0x3fffffff,
	//COR_PRF_MONITOR_IMMUTABLE	= COR_PRF_MONITOR_CODE_TRANSITIONS | COR_PRF_MONITOR_REMOTING | COR_PRF_MONITOR_REMOTING_COOKIE | COR_PRF_MONITOR_REMOTING_ASYNC | COR_PRF_MONITOR_GC | COR_PRF_ENABLE_REJIT | COR_PRF_ENABLE_INPROC_DEBUGGING | COR_PRF_ENABLE_JIT_MAPS | COR_PRF_DISABLE_OPTIMIZATIONS | COR_PRF_DISABLE_INLINING | COR_PRF_ENABLE_OBJECT_ALLOCATED | COR_PRF_ENABLE_FUNCTION_ARGS | COR_PRF_ENABLE_FUNCTION_RETVAL | COR_PRF_ENABLE_FRAME_INFO | COR_PRF_ENABLE_STACK_SNAPSHOT | COR_PRF_USE_PROFILE_IMAGES

	// set the event mask 
	DWORD eventMask = (DWORD)(COR_PRF_MONITOR_ENTERLEAVE);
	return _ICorProfilerInfo2->SetEventMask(COR_PRF_MONITOR_ENTERLEAVE | 
											COR_PRF_ENABLE_FUNCTION_RETVAL |  
											COR_PRF_ENABLE_FUNCTION_ARGS |
											COR_PRF_ENABLE_FRAME_INFO);
}

STDMETHODIMP CProfiler::Initialize(IUnknown *pICorProfilerInfoUnk)
{
	_cProfilerGlobalHandler = this;
	// Query for ICorProfiler2 object
	HRESULT hr =  pICorProfilerInfoUnk->QueryInterface(IID_ICorProfilerInfo2, (LPVOID*) &_ICorProfilerInfo2);
	// Register for recieving specified events
	hr = SetEventMask();

	// Register handlers for function enter/leave events
	_ICorProfilerInfo2->SetEnterLeaveFunctionHooks2( &FunctionEnterHandler, &FunctionLeaveHandler, NULL);
	
	
	// Setting function mapper
	_ICorProfilerInfo2->SetFunctionIDMapper(FunctionMapper);
	LOG4CXX_INFO(myMainLogger, "Program has started");
	
	LOG4CXX_DEBUG(myMainLogger, "Initializing grammar parser...");

	LOG4CXX_TRACE(myMainLogger, "Initializing attribute reader");
	attributeReader = new CAttributeReader();
	functionsMap = new map<FunctionID, CClousureEvaluator*>();

    return S_OK;
} 

STDMETHODIMP CProfiler::Shutdown() {
//	delete grammarParser;
//	delete attributeReader;
	
	LOG4CXX_INFO(myMainLogger, "Program has ended");
	return S_OK;
}
