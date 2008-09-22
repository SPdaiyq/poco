//
// X509Certificate.cpp
//
// $Id: //poco/1.3/NetSSL_OpenSSL/src/X509Certificate.cpp#3 $
//
// Library: NetSSL_OpenSSL
// Package: SSLCore
// Module:  X509Certificate
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "Poco/Net/X509Certificate.h"
#include "Poco/Net/SSLException.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/SecureSocketImpl.h"
#include "Poco/TemporaryFile.h"
#include "Poco/FileStream.h"
#include "Poco/StreamCopier.h"
#include <openssl/pem.h>


namespace Poco {
namespace Net {

X509Certificate::X509Certificate(std::istream& str):
	_issuerName(),
	_subjectName(),
	_pCert(0),
	_file()
{
	Poco::TemporaryFile certFile;
	
	if (!certFile.createFile())
		throw Poco::FileException("No temporary file could be created for X509Certificate!");
	_file = certFile.path();
	Poco::FileOutputStream fout(_file);
	Poco::StreamCopier::copyStream(str, fout);
	fout.close();
	
	BIO *fp=BIO_new(BIO_s_file());
	const char* pFN = _file.c_str();
	BIO_read_filename(fp, (void*)pFN);
	if (!fp)
		throw Poco::PathNotFoundException("Failed to open temporary file for X509Certificate");
	try
	{
		_pCert = PEM_read_bio_X509(fp,0,0,0);
	}
	catch(...)
	{
		BIO_free(fp);
		throw;
	}
	if (!_pCert)
		throw SSLException("Faild to load certificate from " + _file);
	initialize();
}

X509Certificate::X509Certificate(const std::string& file):
	_issuerName(),
	_subjectName(),
	_pCert(0),
	_file(file)
{
	BIO *fp=BIO_new(BIO_s_file());
	const char* pFN = file.c_str();
	BIO_read_filename(fp, (void*)pFN);
	if (!fp)
		throw Poco::PathNotFoundException("Failed to open " + file);
	try
	{
		_pCert = PEM_read_bio_X509(fp,0,0,0);
	}
	catch(...)
	{
		BIO_free(fp);
		throw;
	}
	if (!_pCert)
		throw SSLException("Faild to load certificate from " + file);
	initialize();
}


X509Certificate::X509Certificate(X509* pCert):
	_issuerName(),
	_subjectName(),
	_pCert(pCert),
	_file()
{
	poco_check_ptr(_pCert);
	initialize();
}


X509Certificate::X509Certificate(const X509Certificate& cert):
	_issuerName(cert._issuerName),
	_subjectName(cert._subjectName),
	_pCert(cert._pCert),
	_file(cert._file)
{
	if (!_file.empty())
		_pCert = X509_dup(_pCert);
}


X509Certificate& X509Certificate::operator=(const X509Certificate& cert)
{
	if (this != &cert)
	{
		X509Certificate c(cert);
		swap(c);
	}
	return *this;
}


void X509Certificate::swap(X509Certificate& cert)
{
	using std::swap;
	swap(cert._file, _file);
	swap(cert._issuerName, _issuerName);
	swap(cert._subjectName, _subjectName);
	swap(cert._pCert, _pCert);
}


X509Certificate::~X509Certificate()
{
	if (!_file.empty() && _pCert)
		X509_free(_pCert);
}


void X509Certificate::initialize()
{
	char data[256];
	X509_NAME_oneline(X509_get_issuer_name(_pCert), data, 256);
	_issuerName = data;
	X509_NAME_oneline(X509_get_subject_name(_pCert), data, 256);
	_subjectName = data;
}


bool X509Certificate::verify(const std::string& hostName, Poco::SharedPtr<Context> ptr)
{
	X509* pCert = X509_dup(_pCert);
	return (X509_V_OK == SecureSocketImpl::postConnectionCheck(ptr, pCert, hostName));
}


} } // namespace Poco::Net
