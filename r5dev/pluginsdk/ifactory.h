#pragma once

struct FactoryInfo_t;
class CMemory;

// TODO: Make this abstract and make it base class of CFactory.
class IFactory
{
public:
	virtual void AddFactory(const string& svFactoryName, void* pFactory) = 0;
	virtual void AddFactory(FactoryInfo_t factoryInfo) = 0;
	virtual size_t GetVersionIndex(const string& svInterfaceName) const = 0;
	virtual void GetFactoriesFromRegister(void) = 0;
	virtual CMemory GetFactoryPtr(const string& svFactoryName, bool versionLess = true) const = 0;
	virtual const char* GetFactoryFullName(const string& svFactoryName) const = 0;
};
