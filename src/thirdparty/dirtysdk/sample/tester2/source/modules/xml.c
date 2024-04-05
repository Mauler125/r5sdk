/*H********************************************************************************/
/*!
    \File xml.c

    \Description
        Test the XML parser.

    \Copyright
        Copyright (c) 1999-2005 Electronic Arts Inc.

    \Version 10/04/1999 (gschaefer) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/xml/xmlparse.h"
#include "DirtySDK/xml/xmlformat.h"
#include "testermodules.h"

#include "libsample/zlib.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

// Variables

static const char _CmdXml_strData1[] =
"<?xml version=\"1.0\" standalone=\"yes\"?>"
"<!DOCTYPE get-account-info-reply ["
"   <!ELEMENT get-account-info-reply (status,message,"
"   first-name?,last-name?,user-status,gender?,email,parental-email?, bdateday?,"
"bdatemonth,bdateyear,country,ea-email-flag?,third-party-share-flag?)> "
"   <!ATTLIST get-account-info-reply dtdversion CDATA #REQUIRED>"
"   <!ELEMENT status (#PCDATA)>"
"   <!ELEMENT message (#PCDATA)> "
"   <!ELEMENT first-name (#PCDATA)> "
"   <!ELEMENT last-name (#PCDATA)> "
"   <!ELEMENT user-status (#PCDATA)>"
"   <!ELEMENT gender (#PCDATA)> "
"   <!ELEMENT email (#PCDATA)> "
"   <!ELEMENT parental-email (#PCDATA)>"
"   <!ELEMENT postal-code (#PCDATA)> "
"   <!ELEMENT country (#PCDATA)> "
"   <!ELEMENT ea-email-flag (#PCDATA)> "
"   <!ELEMENT third-party-share-flag (#PCDATA)> "
"   <!ELEMENT bdateday (#PCDATA)>"
"   <!ELEMENT bdatemonth(#PCDATA)>"
"   <!ELEMENT bdateyear (#PCDATA)>"
"]> "
"<get-account-info-reply dtdversion=\"1.0\">"
"   <id>123456789012345678</id>"
"   <status>0</status>"
"   <message>Successfully Retrieved Data</message>"
"   <first-name>Dudeman</first-name>"
"   <last-name>Dudeman</last-name>"
"   <user-status>0</user-status>"
"   <gender>1</gender>"
"   <email>dudeman@ea.com</email>"
"   <bdateday >25</bdateday >"
"   <bdatemonth >12</bdatemonth >"
"   <bdateyear >1970</bdateyear >"
"   <country>USA</country>"
"   <ea-email-flag>1</ea-email-flag>"
"   <third-party-share-flag>0</third-party-share-flag>"
"</get-account-info-reply >";

static const char _CmdXml_strData2[] =
"<TOURNAMENT id=\"1270\" name=\"blahblahblah\" type=\"knockout\" status=\"started\" maxMembers=\"16\" numMembers=\"2\" roundDuration=\"86400000\" autofill=\"random\" beginDate=\"1048196738000\" ante=\"0\" public=\"true\" proPlay=\"\" skill=\"\" ranked=\"false\" rounds=\"0\" playoffRounds=\"1\" currentRound=\"0\" playoffs=\"false\" dupTeams=\"false\" ownerID=\"10783615\" owerName=\"easo2\" gameSettings=\"\"> "
"   <MEMBERS> "
"       <MEMBER id=\"10783615\" name=\"easo2\" status=\"active\" team=\"1\" clubId=\"\" pool=\"0\" seed=\"1\" /> "
"       <MEMBER id=\"10783616\" name=\"easo3\" status=\"active\" team=\"22\" clubId=\"\" pool=\"0\" seed=\"2\" /> "
"   </MEMBERS> "
"   <MATCHES> "
"       <MATCH id=\"3536\" round=\"0\" pool=\"1\" slot=\"0\" expireDate=\"1048283138000\" state=\"scheduled\" homeMember=\"10783615\" homeScore=\"0\" awayMember=\"10783616\" awayScore=\"0\" /> "
"   </MATCHES> "
"</TOURNAMENT>";

static const char _CmdXml_strData3[] =
#if 0
"<presence to=\"12286964334@ebisu.com/TestApp1\" type=\"error\">"
"<priority>1</priority>"
"<c hash=\"sha-1\" node=\"http://ea.com/xmpp\" xmlns=\"http://jabber.org/protocol/caps\" ver=\"MzEtlPWUgkcDStAKxLEO0sK7j2M&#x3d;\"/>"
"<error code=\"500\" type=\"wait\"><internal-server-error xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
"<text xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\" xml:lang=\"en\">"
"xml to entity failed, xml is invalid.unexpected element (uri:\"\", local:\"html\"). Expected elements are <{}error>"
//"xml to entity failed, xml is invalid.unexpected element (uri:\"\", local:\"html\"). Expected elements are {}error"
"</text>"
"</error>"
"</presence>"
#elif 0
"<iq id=\"libxmpp2\" to=\"12286964335@ebisu.com/TestApp2\" type=\"result\"/>"
"<iq id=\"libxmpp3\" to=\"12286964335@ebisu.com/TestApp2\" type=\"error\">"
"<pubsub xmlns=\"http://jabber.org/protocol/pubsub\">"
"<publish node=\"http://jabber.org/protocol/nick\">"
"<item>"
"<nick xmlns=\"http://jabber.org/protocol/nick\">"
"My Default Persona</nick>"
"</item>"
"</publish>"
"</pubsub>"
"<error code=\"501\" type=\"cancel\">"
"<feature-not-implemented xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
"<text xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\" xml:lang=\"en\">"
"Feature not supported yet.</text>"
"</error>"
"</iq>"
"<iq id=\"libxmpp4\" to=\"12286964335@ebisu.com/TestApp2\" type=\"error\">"
"<pubsub xmlns=\"http://jabber.org/protocol/pubsub\">"
"<publish node=\"urn:xmpp:avatar:metadata\">"
"<item>"
"<metadata xmlns=\"urn:xmpp:avatar:metadata\"/>"
"</item>"
"</publish>"
"</pubsub>"
"<error code=\"501\" type=\"cancel\">"
"<feature-not-implemented xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
"<text xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\" xml:lang=\"en\">"
"Feature not supported yet.</text>"
"</error>"
"</iq>"
"<iq id=\"libxmpp5\" to=\"12286964335@ebisu.com/TestApp2\" type=\"result\">"
"<query xmlns=\"jabber:iq:roster\"/>"
"</iq>"
"<iq id=\"libxmpp6\" xmlns=\"jabber:client\" type=\"error\" to=\"12286964335@ebisu.com/TestApp2\">"
"<query xmlns=\"jabber:iq:privacy\">"
"<list name=\"blocked_users\"/>"
"</query>"
"<error code=\"404\" type=\"cancel\">"
"<item-not-found xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
"<text xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\" xml:lang=\"en\">"
"Requested list not found.</text>"
"</error>"
"</iq"
#elif 0
"<iq id=\"libxmpp2\" to=\"12286964335@ebisu.com/TestApp2\" type=\"result\"/>"
"<iq id=\"libxmpp3\" to=\"12286964335@ebisu.com/TestApp2\" type=\"error\">"
"<pubsub xmlns=\"http://jabber.org/protocol/pubsub\">"
"<publish node=\"http://jabber.org/protocol/nick\">"
"<item>"
"<nick xmlns=\"http://jabber.org/protocol/nick\">"
"My Default Persona</nick>"
"</item>"
"</publish>"
"</pubsub>"
"<error code=\"501\" type=\"cancel\">"
"<feature-not-implemented xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
"<text xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\" xml:lang=\"en\">"
"Feature not supported yet.</text>"
"</error>"
"</iq>"
"<iq id=\"libxmpp4\" to=\"12286964335@ebisu.com/TestApp2\" type=\"error\">"
"<pubsub xmlns=\"http://jabber.org/protocol/pubsub\">"
"<publish node=\"urn:xmpp:avatar:metadata\">"
"<item>"
"<metadata xmlns=\"urn:xmpp:avatar:metadata\"/>"
"</item>"
"</publish>"
"</pubsub>"
"<error code=\"501\" type=\"cancel\">"
"<feature-not-implemented xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
"<text xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\" xml:lang=\"en\">"
"Feature not supported yet.</text>"
"</error>"
"</iq>"
"<iq id=\"libxmpp6\" xmlns=\"jabber:client\" type=\"error\" to=\"12286964335@ebisu.com/TestApp2\">"
"<query xmlns=\"jabber:iq:privacy\">"
"<list name=\"blocked_users\"/>"
"</query>"
"<error code=\"404\" type=\"cancel\">"
"<item-not-found xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
"<text xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\" xml:lang=\"en\">"
"Requested list not found.</text>"
"</error>"
"</iq>"
"<iq id=\"libxmpp5\" to=\"12286964335@ebisu"
#elif 1
"<iq from='test1@"
"albi.ncogni.to' "
"to='test1@albi.n"
"cogni.to/TestApp"
"1' id='libxmpp5'"
" type='error'><q"
"uery xmlns='jabb"
"er:iq:privacy'><"
"list name='block"
"ed_users'/></que"
"ry><error code='"
"404' type='cance"
"l'><item-not-fou"
"nd xmlns='urn:ie"
"tf:params:xml:ns"
":xmpp-stanzas'/>"
"</error></iq><iq"
" from='test1@alb"
"i.ncogni.to' to="
"'test1@albi.ncog"
"ni.to/TestApp1' "
"id='libxmpp4' ty"
"pe='result'><que"
"ry xmlns='jabber"
":iq:roster'/></i"
"q><iq from='test"
"1@albi.ncogni.to"
"' to='test1@albi"
".ncogni.to/TestA"
"pp1' id='libxmpp"
"3' type='result'"
"><pubsub xmlns='"
"http://jabber.or"
"g/protocol/pubsu"
"b'><publish node"
"='urn:xmpp:avata"
"r:metadata'><ite"
"m id='50542C78C7"
"27A'/></publish>"
"</pubsub></iq>  "
#endif
;


/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function CmdXml

    \Description
        Test the Xml parser

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list
    
    \Output standard return value

    \Version 10/04/1999 (gschaefer)
*/
/********************************************************************************F*/
int32_t CmdXml(ZContext *argz, int32_t argc, char *argv[])
{
    int32_t iIndex;
    char strData[1024];
    const char *pXml;

    for (pXml = _CmdXml_strData3; pXml != NULL; pXml = XmlSkip(pXml))
    {
        ZPrintf("%s\n", XmlComplete(pXml) ? "valid" : "invalid");
    }

    XmlPrintFmt((_CmdXml_strData3, ""));

    // check usage
    if (argc < 2)
    {
        ZPrintf("   test xmlparse and xmlformat modules\n");
        ZPrintf("   usage: %s testnum [1..3]\n", argv[0]);
        return(0);
    }

    // get the index
    iIndex = atoi(argv[1]);

    // do the tests
    if (iIndex == 1)
    {
        XmlContentGetString(XmlFind(_CmdXml_strData1, "get-account-info-reply.email"), strData, sizeof(strData), "");
        ZPrintf("email=%s\n", strData);

        ZPrintf("id=%lld\n", XmlContentGetInteger64(XmlFind(_CmdXml_strData1, "get-account-info-reply.id"), -1));
        ZPrintf("bdateyear=%d\n", XmlContentGetInteger(XmlFind(_CmdXml_strData1, "get-account-info-reply.bdateyear"), -1));
    }
    if (iIndex == 2)
    {
        pXml = XmlFind(_CmdXml_strData2, "TOURNAMENT.MATCHES");
        ZPrintf("hit=%s\n", pXml);

        ZPrintf("beginDate=%lld\n", XmlAttribGetInteger64(XmlFind(_CmdXml_strData2, "TOURNAMENT"), "beginDate", -1));
        ZPrintf("id=%d\n", XmlAttribGetInteger(XmlFind(_CmdXml_strData2, "TOURNAMENT"), "id", -1));
    }
    if (iIndex == 3)
    {
        char strBuffer[1024];
        
        // encode some XML
        XmlInit(strBuffer, sizeof(strBuffer), XML_FL_WHITESPACE);

        XmlTagStart(strBuffer, "TestFormat1");
        XmlElemAddString(strBuffer, "strText", "Some chars that should be encoded: =, <, >, &, \", \x7f");
        XmlTagEnd(strBuffer);
        
        XmlTagStart(strBuffer, "TestFormat2");
        XmlElemAddString(strBuffer, "strText", "Some chars that should not be encoded: \t, \r, \n");
        XmlTagEnd(strBuffer);
        
        XmlFinish(strBuffer);
        
        ZPrintf("encoded xml:\n------------\n%s\n-----------\n", strBuffer);
        
        // now parse the encoded xml
        XmlContentGetString(XmlFind(strBuffer, "TestFormat1.strText"), strData, sizeof(strData), "");
        ZPrintf("test1=%s\n", strData);
        
        XmlContentGetString(XmlFind(strBuffer, "TestFormat2.strText"), strData, sizeof(strData), "");
        ZPrintf("test2=%s\n", strData);
    }

    return(0);
}


