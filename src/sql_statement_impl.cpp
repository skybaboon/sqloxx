/*
 * This file is part of the Sqloxx project and is distributed under the
 * terms of the license contained in the file LICENSE.txt distributed
 * with this package.
 * 
 * Author: Matthew Harvey <matthew@matthewharvey.net>
 *
 * Copyright (c) 2012-2013, Matthew Harvey.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "detail/sql_statement_impl.hpp"
#include "detail/sqlite3.h" // Compiling directly into build
#include "detail/sqlite_dbconn.hpp"
#include <jewel/assert.hpp>
#include <jewel/exception.hpp>
#include <jewel/log.hpp>
#include <string>

using std::string;

namespace sqloxx
{
namespace detail
{



SQLStatementImpl::SQLStatementImpl
(	SQLiteDBConn& p_sqlite_dbconn,
	string const& str
):
	m_statement(0),
	m_sqlite_dbconn(p_sqlite_dbconn),
	m_is_locked(false)
{
	if (!p_sqlite_dbconn.is_valid())
	{
		JEWEL_THROW
		(	InvalidConnection,
			"Attempt to initialize SQLStatementImpl with invalid "
			"DatabaseConnection."
		);
	}
	char const* cstr = str.c_str();
	char const** tail = &cstr;
	JEWEL_ASSERT (p_sqlite_dbconn.is_valid());
	throw_on_failure
	(	sqlite3_prepare_v2
		(	m_sqlite_dbconn.m_connection,
			cstr,
			str.length() + 1,
			&m_statement,
			tail
		)
	);
	for (char const* it = *tail; *it != '\0'; ++it)
	{
		switch (*it)
		{
		case ';':
		case ' ':
			// The character is harmless.
			break;
		default:
			// The character is bad.
			sqlite3_finalize(m_statement);  // Always succeeds.
			m_statement = 0;
			// Note this will have thrown already if first statement is
			// ungrammatical.
			JEWEL_THROW
			(	TooManyStatements,
				"Compound SQL statement passed to constructor of "
				"SQLStatementImpl - which can handle only single statements."
			);
		}
	}
	return;
}


SQLStatementImpl::~SQLStatementImpl()
{
	if (m_statement)
	{
		sqlite3_finalize(m_statement);
		m_statement = 0;
	}
}


void
SQLStatementImpl::check_column(int index, int value_type)
{
	int const num_columns = sqlite3_column_count(m_statement);
	if (num_columns == 0)
	{
		JEWEL_THROW(NoResultRowException, "Result row not available.");
	}
	if (index >= num_columns)
	{
		JEWEL_THROW(ResultIndexOutOfRange, "Index is out of range.");
	}
	if (index < 0)
	{
		JEWEL_THROW(ResultIndexOutOfRange, "Index is negative.");
	}
	if (value_type != sqlite3_column_type(m_statement, index))
	{
		JEWEL_THROW
		(	ValueTypeException,
			"Value type at index does not match specified value type."
		);
	}
	return;
}


void
SQLStatementImpl::throw_on_failure(int errcode)
{
	m_sqlite_dbconn.throw_on_failure(errcode);
	return;
}

bool
SQLStatementImpl::step()
{
	if (!m_sqlite_dbconn.is_valid())
	{
		JEWEL_THROW(InvalidConnection, "Invalid database connection.");
	}
	int code = SQLITE_OK;
	try
	{
		// Intentional assignment
		throw_on_failure(code = sqlite3_step(m_statement));
	}
	catch (SQLiteException&)
	{
		reset();
		clear_bindings();
		throw;
	}
	switch (code)
	{
	case SQLITE_DONE:

		// After SQLite version 3.6.23.1, the statement is
		// reset automatically.
		#if SQLITE_VERSION_NUMBER < 3007000
			sqlite3_reset(m_statement);
		#endif

		return false;
		JEWEL_HARD_ASSERT (false);  // Execution never reaches here
	case SQLITE_ROW:
		return true;
		JEWEL_HARD_ASSERT (false);  // Execution never reaches here
	default:
		;
		// Do nothing
	}
	JEWEL_HARD_ASSERT (false);  // Execution should never reach here.
	return false;  // Silence compiler re. return from non-void function. 
}


void
SQLStatementImpl::step_final()
{
	if (step())
	{
		reset();
		JEWEL_THROW
		(	UnexpectedResultRow,
			"Statement yielded a result set when none was expected."
		);
	}
	return;
}


int
SQLStatementImpl::parameter_index
(	string const& parameter_name
)
const
{
	int const ret = sqlite3_bind_parameter_index
	(	m_statement,
		parameter_name.c_str()
	);
	if (ret == 0) 
	{
		try
		{
		}
		catch (...)
		{
		}
		JEWEL_THROW(SQLiteException, "Could not find parameter index.");
	}
	JEWEL_ASSERT (ret > 0);
	return ret;
}



}  // namespace detail
}  // namespace sqloxx
