#ifndef _SQCLOSURE_H_
#define _SQCLOSURE_H_

#define SQ_MALLOC(size) new char[size]
struct SQFunctionProto;

struct SQClosure
{
    void Release() {}
    void Mark() {}
    void Finalize() {}

    SQFunctionProto* _function;
    SQObjectPtr _env;
    std::vector<SQObjectPtr> _outervalues;
    std::vector<SQObjectPtr> _defaultparams;

    SQClosure(SQSharedState* ss, SQFunctionProto* func) : _function(func) {}

    static SQClosure* Create(SQSharedState* ss, SQFunctionProto* func) {
        void* mem = SQ_MALLOC(sizeof(SQClosure));
        if (!mem) return nullptr;
        return new (mem) SQClosure(ss, func);
    }

  
};


struct SQNativeClosure : public SQClosure {
    SQFUNCTION _nativeFunction;
    SQObjectPtr _name;

    SQNativeClosure(SQSharedState* ss, SQFUNCTION func) : SQClosure(ss, nullptr), _nativeFunction(func) {}

    static SQNativeClosure* Create(SQSharedState* ss, SQFUNCTION func) {
        return new SQNativeClosure(ss, func);
    }
};



#endif //_SQCLOSURE_H_
