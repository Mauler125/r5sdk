#include "stdafx.h"
#include "Uri.h"

namespace Net
{
	Uri::Uri(const char* Url)
		: Uri(String(Url))
	{
	}

	Uri::Uri(const String& Url)
		: InternetPort(InternetPortType::Default), Host(""), Path("")
	{
		this->ParseUri(Url);
	}

	String Uri::GetUrl()
	{
		auto Base = (InternetPort == InternetPortType::Http) ? "http://" : "https://";

		return Base + Host + Path;
	}

	void Uri::ParseUri(const String& Url)
	{
		auto LowerCaseUrl = Url.ToLower();
		uint32_t ParsePoint = 0;

		if (LowerCaseUrl.StartsWith("https://"))
		{
			this->InternetPort = InternetPortType::Https;
			ParsePoint = 8;
		}
		else if (LowerCaseUrl.StartsWith("http://"))
		{
			this->InternetPort = InternetPortType::Http;
			ParsePoint = 7;
		}
		else if (LowerCaseUrl.StartsWith("//"))
		{
			ParsePoint = 2;
		}

		auto PathStart = Url.IndexOf("/", ParsePoint);

		if (PathStart != String::InvalidPosition)
		{
			this->Host = Url.SubString(ParsePoint, PathStart - ParsePoint);
			this->Path = Url.SubString(PathStart + 1);
		}
		else
		{
			this->Host = Url.SubString(ParsePoint);
		}
	}
}
