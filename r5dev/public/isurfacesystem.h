#ifndef ISURFACESYSTEM_H
#define ISURFACESYSTEM_H

class ISurface
{
public:
    virtual ~ISurface() { };
	virtual bool Init() = 0;
	virtual void Think() = 0;
	virtual void RunFrame() = 0;
	virtual void RunTask() = 0;
	virtual void DrawSurface() = 0;
	virtual void SetStyleVar() = 0;
};

struct CSuggest
{
    CSuggest(const string& svName, int nFlags)
    {
        m_svName = svName;
        m_nFlags = nFlags;
    }
    bool operator==(const string& a) const
    {
        return m_svName.compare(a) == 0;
    }
    bool operator<(const CSuggest& a) const
    {
        return m_svName < a.m_svName;
    }
    string m_svName;
    int m_nFlags;
};

#endif // ISURFACESYSTEM_H
