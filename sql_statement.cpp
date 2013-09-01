// Copyright (c) 2013, Matthew Harvey. All rights reserved.

#include "sql_statement.hpp"
#include "database_connection.hpp"
#include "detail/sql_statement_impl.hpp"
#include <jewel/log.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

using boost::shared_ptr;
using std::endl;  // for debug logging
using std::string;


namespace sqloxx
{



SQLStatement::~SQLStatement()
{
	m_sql_statement->reset();
	m_sql_statement->clear_bindings();
	m_sql_statement->unlock();
}



template <>
int
SQLStatement::extract<int>(int index)
{
	return m_sql_statement->extract<int>(index);
}


template <>
long
SQLStatement::extract<long>(int index)
{
	return m_sql_statement->extract<long>(index);
}

template <>
long long
SQLStatement::extract<long long>(int index)
{
	return m_sql_statement->extract<long long>(index);
}


// All these total specialisations of SQLStatement::extract
// are to avoid having to put the call to SQLStatementImpl::extract in the
// header - which would introduce unwanted compilation dependencies.

template <>
double
SQLStatement::extract<double>(int index)
{
	return m_sql_statement->extract<double>(index);
}


template <>
string
SQLStatement::extract<string>(int index)
{
	return m_sql_statement->extract<string>(index);
}


void
SQLStatement::bind(string const& parameter_name, int x)
{
	m_sql_statement->bind(parameter_name, x);
	return;
}


void
SQLStatement::bind(string const& parameter_name, long x)
{
	m_sql_statement->bind(parameter_name, x);
	return;
}

void
SQLStatement::bind(string const& parameter_name, long long x)
{
	m_sql_statement->bind(parameter_name, x);
	return;
}


void
SQLStatement::bind(string const& parameter_name, double x)
{
	m_sql_statement->bind(parameter_name, x);
}

void
SQLStatement::bind(string const& parameter_name, string const& x)
{
	m_sql_statement->bind(parameter_name, x);
	return;
}


bool
SQLStatement::step()
{
	return m_sql_statement->step();
}


void
SQLStatement::step_final()
{
	m_sql_statement->step_final();
	return;
}


void
SQLStatement::reset()
{
	m_sql_statement->reset();
	return;
}


void
SQLStatement::clear_bindings()
{
	m_sql_statement->clear_bindings();
	return;
}



}  // namespace sqloxx
