#ifndef GUARD_sql_statement_impl_hpp
#define GUARD_sql_statement_impl_hpp

// Hide from Doxygen
/// @cond

/** \file sql_statement_impl.hpp
 *
 * \brief Header file pertaining to SQLStatementImpl class.
 *
 * \author Matthew Harvey
 * \date 04 July 2012.
 *
 * Copyright (c) 2012, Matthew Harvey. All rights reserved.
 */


#include "../sqloxx_exceptions.hpp"
#include <jewel/checked_arithmetic.hpp>
#include "sqlite3.h"  // Compiling directly into build
#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>
#include <boost/static_assert.hpp>
#include <limits>
#include <string>
#include <vector>



namespace sqloxx
{
namespace detail
{

// Forward declaration
class SQLiteDBConn;

/**
 * Wrapper class for sqlite_stmt*. This class is should not be
 * used except internally by the Sqloxx library. SQLStatementImpl instances
 * are themselves encapsulated by SQLStatement instances.
 *
 * @todo Could long long overflow the SQLite 64-bit integer type on some
 * platforms? Possibly. We have a static assertion (below) to break
 * compilation on such platforms. This is unsatisfying though.
 */
class SQLStatementImpl:
	private boost::noncopyable
{
public:

	/**
	 * Creates an object encapsulating a SQL statement.
	 *
	 * @param str is the text of a single SQL statement. It can be terminated
	 * with any mixture of semicolons and/or spaces (but not other forms
	 * of whitespace).
	 *
	 * @throws InvalidConnection if the database connection passed to
	 * \c dbconn is invalid.
	 *
	 * @throws SQLiteException or an exception derived therefrom, if
	 * the database connection is valid, but the statement could not
	 * be properly prepared by SQLite.
	 *
	 * @throws TooManyStatements if the first purported SQL statement
	 * in str is syntactically acceptable to SQLite, <em>but</em> there
	 * are characters in str after this statement, other than ';' and ' '.
	 * This includes the case where there are further syntactically
	 * acceptable SQL statements after the first one - as each SQLStatementImpl
	 * can encapsulate only one statement.
	 */
	SQLStatementImpl(SQLiteDBConn& p_sqlite_dbconn, std::string const& str);

	~SQLStatementImpl();

	/**
	 * Wrapper around SQLite bind functions.
	 *
	 * These throw \c SQLiteException, or an exception derived therefrom,
	 * if SQLite could not properly bind the statement.
	 * 
	 * Currently the following types for T are supported:\n
	 * int\n
	 * long\n
	 * long long\n
	 * double\n
	 * std::string\n
	 */
	template <typename T>
	void bind(std::string const& parameter_name, T const& x);

	/**
	 * Where a SQLStatementImpl has a result set available,
	 * this function (template) can be used to extract the value at
	 * the \c indexth column of the current row (where \c index starts
	 * counting at 0).
	 *
	 * Currently the following types for T are supported:\n
	 *	long\n
	 *	long long\n
	 *	int\n
	 *	double\n
	 *	std::string\n
	 * 
	 * @param index is the column number (starting at 0) from which to
	 * read the value.
	 * 
	 * @throws ResultIndexOutOfRange if \c index is out of range.
	 *
	 * @throws ValueTypeException if the requested column contains a type that
	 * is incompatible with T.
	 */
	template <typename T>
	T extract(int index);

	/**
	 * Wraps sqlite3_step
	 * Returns true as only long as there are further steps to go (i.e. result
	 * rows to examine).
	 *
	 * On stepping beyond the last result row, step() will return false.
	 * The statement will then be automatically reset (see reset()).
	 *
	 * @throws SQLiteException or some exception deriving therefrom, if an
	 * error occurs. This function should almost never throw, but it is
	 * possible something will fail as the statement is being executed, in
	 * which the resulting SQLite error condition will trigger the
	 * corresponding exception class.
	 */
	bool step();

	/**
	 * Wraps sqlite3_step. Similar to \c step except that it throws an
	 * exception if a result row still remains after calling. That is,
	 * it is equivalent to calling:\n
	 * \c if (step()) throw UnexpectedResultRow("...");\n
	 *
	 * @throws UnexpectedResultRow if a result set is returned.
	 * 
	 * @throws SQLiteException or an exception derived therefrom if there
	 * is any other error in executing the statement.
	*/
	void step_final();

	/**
	 * Resets the statement, freeing bound parameters ready for
	 * subsequent re-binding and re-execution.
	 *
	 * Does not throw.
	 */
	void reset();

	/**
	 * Clears the parameter bindings from the statement, setting all
	 * to NULL. This is a wrapper for sqlite3_clear_bindings.
	 * Does not throw.
	 */
	void clear_bindings();

	/**
	 * @returns true if and only if the statement is currently
	 * in use by way of a SQLStatement. Does not throw.
	 */
	bool is_locked() const;

	/**
	 * Locks the statement, indicating that is currently in
	 * use. Does not throw.
	 */
	void lock();

	/**
	 * Unlocks the statement, indicating that it is now available
	 * for use. Does not throw.
	 */
	void unlock();

	/**
	 * Mirrors sqloxx::detail::SQLiteDBConn::throw_on_failure, and
	 * throws the same exceptions under the same circumstances.
	 */
	void throw_on_failure(int errcode);

private:
	
	/**
	 * @parameter_name is the name of a column in the result set.
	 * 
	 * @throws NoMatchingColumnException if \c parameter_name does not
	 * name a column in the result set.
	 */
	int parameter_index(std::string const& column_name) const;

	/**
	 * Checks whether a column is available for extraction at
	 * index \c index, of type \c value_type, and throws an
	 * exception if not.
	 *
	 * @param index Position of column (starts from zero) in result
	 * row.
	 * 
	 * @param value_type Should be a SQLite value type code, i.e. one of:\n
	 * 	SQLITE_INTEGER, SQLITE_FLOAT, SQLITE_TEXT, SQLITE_BLOB, SQLITE_NULL.
	 *
	 * @throws NoResultRowException if there are no results available for
	 * extraction.
	 *
	 * @throws ResultIndexOutOfRange if \c index is negative or is otherwise
	 * out of range.
	 *
	 * @throws ValueTypeException if the value at position \c index is not of
	 * value type \c value_type.
	 */
	void check_column(int index, int value_type);

	void do_bind(std::string const& parameter_name, int x);
	void do_bind(std::string const& parameter_name, long x);
	void do_bind(std::string const& parameter_name, long long x);
	void do_bind(std::string const& parameter_name, double x);
	void do_bind(std::string const& parameter_name, std::string const& x);
	// Not implemented for other types, so capture here to prevent compilation
	// if other types passed
	template <typename T>
	void do_bind(std::string const& parameter_name, T t);


	sqlite3_stmt* m_statement;
	SQLiteDBConn& m_sqlite_dbconn;
	bool m_is_locked;
};


// FUNCTION TEMPLATE DEFINITIONS AND INLINE FUNCTIONS


template <typename T>
inline
void
SQLStatementImpl::bind(std::string const& parameter_name, T const& x)
{
	try
	{
		do_bind(parameter_name, x);
	}
	catch (SQLiteException&)
	{
		reset();
		clear_bindings();
		throw;
	}
	return;
}


template <>
inline
int
SQLStatementImpl::extract<int>(int index)
{
	check_column(index, SQLITE_INTEGER);
	return sqlite3_column_int(m_statement, index);
}

template <>
inline
long
SQLStatementImpl::extract<long>(int index)
{
	check_column(index, SQLITE_INTEGER);
	return sqlite3_column_int64(m_statement, index);
}


// long long is guaranteed to be at least 8 bytes. But
// if it's \e greater than 8 bytes, this causes a danger
// of overflow of SQLite's 64-bit integer type column - in
// which we will want to store values of this type.
// Compilation should fail in this case.
BOOST_STATIC_ASSERT(sizeof(long long) == 8);


template <>
inline
long long
SQLStatementImpl::extract<long long>(int index)
{
	check_column(index, SQLITE_INTEGER);
	return sqlite3_column_int64(m_statement, index);
}

template <>
inline
double
SQLStatementImpl::extract<double>(int index)
{
	check_column(index, SQLITE_FLOAT);
	return sqlite3_column_double(m_statement, index);
}

template <>
inline
std::string
SQLStatementImpl::extract<std::string>(int index)
{
	check_column(index, SQLITE_TEXT);
	const unsigned char* begin = sqlite3_column_text(m_statement, index);
	const unsigned char* end = begin;
	while (*end != '\0') ++end;
	return std::string(begin, end);
}


inline
void
SQLStatementImpl::reset()
{
	if (m_statement)
	{
		sqlite3_reset(m_statement);
	}
	return;
}


inline
void
SQLStatementImpl::clear_bindings()
{
	if (m_statement)
	{
		sqlite3_clear_bindings(m_statement);
	}
	return;
}


inline
bool
SQLStatementImpl::is_locked() const
{
	return m_is_locked;
}

inline
void
SQLStatementImpl::lock()
{
	m_is_locked = true;
	return;
}

inline
void
SQLStatementImpl::unlock()
{
	m_is_locked = false;
	return;
}


}  // namespace detail
}  // namespace sqloxx


/// @endcond
// End hiding from Doxygen

#endif  // GUARD_sql_statement_impl.hpp

