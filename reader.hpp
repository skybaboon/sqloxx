#ifndef GUARD_reader_hpp
#define GUARD_reader_hpp

#include "general_typedefs.hpp"
#include "sqloxx_exceptions.hpp"
#include "sql_statement.hpp"
#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <cassert>
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace sqloxx
{


// READER

/**
 * @todo Update documentation and tests given new container-like
 * interface. Note the increased opportunities for undefined behaviour
 * now that the internal list is wholly created on construction - stuff
 * may be deleted, added or modified between when we create the Reader
 * and when we iterate over it.
 *
 * Class template each instantiation of which represents a container of
 * objects loaded from a database by an SQL "select" statement,
 * where the field being selected is the primary key field for type T as
 * it is stored in the database.
 *
 * Reader classes provide a higher-level, cleaner interface for
 * client code wishing to traverse a database table without having to
 * use SQL directly.
 * Each instantiation of Reader represents wraps a particular "select"
 * statement that is used to traverse a table or view in the database.
 * 
 * Client code may use the Reader class in one of two ways:\n
 * (1) Use the Reader class as is, instantiating for a particular
 * type T and database connection type Connection. A Reader can be
 * constructed using the default constructor, or by passing a
 * different string to the p_statement_text parameter of the constructor;\n or
 * (2) Inherit from Reader<T, Connection>, but provide an alternative
 * constructor, that takes a Connection parameter, which it passes up to
 * the base Reader constructor, along with a custom string for
 * p_statement_text
 * that is fixed for that derived class. This enables a given derived Reader
 * class to use a fixed selection statement for all instances of
 * that class.
 *
 * Parameter templates:\n
 * T should be a class
 * which is persisted in the database with a 
 * primary key is of type sqloxx::Id, and which has a
 * static function of the form:\n
 * T create_unchecked(Connection, sqloxx::Id).
 * Connection should be a class inheriting from DatabaseConnection,
 * for which identity_map<T, Connection>() is defined to return
 * a reference to the IdentityMap for that type T for that
 * Connection.
 */
template <typename T, typename Connection>
class Reader:
	private boost::noncopyable
{
public:


	typedef typename std::list<T> Container;
	typedef typename Container::const_iterator const_iterator;
	typedef typename Container::iterator iterator;
	typedef typename Container::size_type size_type;
	typedef typename Container::difference_type difference_type;
	typedef T value_type;
	typedef typename Container::const_reverse_iterator const_reverse_iterator;
	typedef typename Container::reverse_iterator reverse_iterator;
	
	// TODO Figure out whether the these typedefs are necessary
	typedef typename Container::allocator_type allocator_type;
	typedef typename Container::const_pointer const_pointer;
	typedef typename Container::pointer pointer;

	/**
	 * Construct a Reader from a database connection of type
	 * Connection.
	 *
	 * @param p_selector SQL statement string that will select records
	 * representing instances of type T, from the database.
	 * The default string can be seen in the function signature. If a
	 * different string is provided, it should be such that it selects
	 * only a single column, such that that column is the primary
	 * key for class T, from which instances of Handle<T> can be
	 * constructed.
	 *
	 * @param p_database_connection instance of Connection.
	 *
	 * @throws InvalidConnection if p_database_connection is an
	 * invalid database connection (i.e. if p_database_connection.is_valid()
	 * returns false).
	 *
	 * @throws SQLiteException, or an exception derived therefrom, if there
	 * is some problem in constructing the underlying SQL statement,
	 * that results
	 * in a SQLite error code being returned.
	 *
	 * @throws std::bad_alloc in the unlikely event of a memory allocation
	 * error in constructing the underlying SQL statement.
	 * 
	 * @throws TooManyStatements if the first purported SQL statement
	 * in p_selector is syntactically acceptable to SQLite, <em>but</em> there
	 * are characters in p_selector after this statement,
	 * other than ';' and ' '.
	 * This includes the case where there are further syntactically
	 * acceptable SQL statements after the first one.
	 *
	 * Exception safety: <em>strong guarantee</em>.
	 */
	explicit Reader
	(	Connection& p_database_connection,
		std::string const& p_statement_text =
		(	"select " + T::primary_key_name() +
			" from " + T::primary_table_name()
		)
	);

	/**
	 * @todo Document and test.
	 */
	size_type size() const;

	/**
	 * @todo Document and test.
	 */
	const_iterator begin() const;

	/**
	 * @todo Document and test.
	 */
	iterator begin();

	/**
	 * @todo Document and test.
	 */
	const_iterator end() const;

	/**
	 * @todo Document and test.
	 */
	iterator end();

protected:
	/**
	 * @todo Document and test this constructor, and reflect in the
	 * general class documentation as well.
	 *
	 * @todo It seems sucky that SQLStatement is a non-const ref.
	 * If SQLStatement had a copy constructor, it would probably fix
	 * this.
	 */
	Reader(Connection& p_database_connection, SQLStatement& p_statement);

private:	

	/**
	 * Advances the Reader to the next row into the result set.
	 * When the Reader
	 * is first constructed, it is notionally positioned just \e before
	 * the first result row. The first call to read() moves it "onto" the
	 * first result row (if there is one). Etc. Once it is on the final
	 * result row, calling
	 * read() again will result in it moving notionally one beyond the result
	 * row, and false will be returned.
	 * Calling read yet again will then result in the reader
	 * cycling back to the
	 * first row, and returning true again (assuming there is a first row).
	 *
	 * @returns true if, on calling read(), the Reader moves onto a result
	 * row; otherwise, returns false.
	 *
	 * @throws InvalidConnection is thrown if the database connection is
	 * invalid. If this occurs, the state of the Reader
	 * will be the same as just before the throwing call to read() was
	 * made.
	 *
	 * @throws SQLiteException or some exception deriving
	 * therefrom, if an error occurs that results in a SQLite error
	 * code. This should almost never occur. If it does occur, the
	 * Reader will be reset to the state it was in just after
	 * construction - i.e. just before it read the first result row.
	 *
	 * Exception safety: <em>basic guarantee</em>.
	 */
	bool read();
	
	/**
	 * Preconditions:\n
	 * The constructor of T should offer the strong guarantee; and\n
	 * The destructor of T should never throw; and\n
	 * T should have a static method of the form
	 * T create_unchecked(Connection&, Id, char).
	 * Typically, T will be
	 * an instantiation of sqloxx::Handle - but it need not
	 * be.
	 *
	 * @returns an instance of T created by calling static method
	 * T::create_unchecked(Connection&, Id), where the Connection passed to
	 * the Reader's constructor is passed to the Connection&
	 * parameter, and the single field being read by the
	 * Reader from the database (which should be the primary
	 * key field for type T) is passed to the Id parameter.
	 *
	 * @throws ResultIndexOutOfRange if there is an error extracting
	 * the result of the underlying SQL statement. This would
	 * generally only occur if an invalid SQL statement was not
	 * passed to the constructor of the Reader, or if the database
	 * is corrupt.
	 *
	 * @throws ValueTypeException if the type of the column being
	 * selected by the underlying SQL statement is not compatible
	 * with the Id type required to initialize a Handle<T>. If a
	 * suitable SQL statement string is passed to the constructor
	 * of the Reader, this should never occur.
	 *
	 * @throws InvalidReader if the Reader is not currently positioned
	 * over a result row.
	 *
	 * @throws std::bad_alloc if the object (instance of T to which we
	 * want a Handle) is not already loaded in the IdentityMap (cache),
	 * and there is a memory allocation failure in the process of loading
	 * and caching the object.
	 *
	 * @throws InvalidConnection if the database connection is invalid.
	 *
	 * @throws SQLiteException, of a derivative thereof,
	 * in the event of an error during execution thrown up by the
	 * underlying SQLite API. Assuming the Reader was initialized with
	 * a valid SQL statement string, this should almost never occur.
	 *
	 * Exception safety: <em>strong guarantee</em>.
	 */
	T item() const;

	void populate_container();

	Container m_container;
	Connection& m_database_connection;
	SQLStatement mutable m_statement;
	bool m_is_valid;

};  // Reader


template <typename T, typename Connection>
void
Reader<T, Connection>::populate_container()
{
	assert (m_container.empty());
	while (this->read())
	{
		m_container.push_back(this->item());
	}
	return;
}


template <typename T, typename Connection>
Reader<T, Connection>::Reader
(	Connection& p_database_connection,
	std::string const& p_statement_text
):
	m_database_connection(p_database_connection),
	m_statement(p_database_connection, p_statement_text),
	m_is_valid(false)
{
	populate_container();


}


template <typename T, typename Connection>
Reader<T, Connection>::Reader
(	Connection& p_database_connection,
	SQLStatement& p_statement
):
	m_database_connection(p_database_connection),
	m_statement(p_statement),
	m_is_valid(false)
{
	populate_container();
}



template <typename T, typename Connection>
inline
typename Reader<T, Connection>::size_type
Reader<T, Connection>::size() const
{
	return m_container.size();
}


template <typename T, typename Connection>
typename Reader<T, Connection>::const_iterator
Reader<T, Connection>::begin() const
{
	return m_container.begin();
}


template <typename T, typename Connection>
inline
typename Reader<T, Connection>::iterator
Reader<T, Connection>::begin()
{
	return m_container.begin();
}


template <typename T, typename Connection>
inline
typename Reader<T, Connection>::const_iterator
Reader<T, Connection>::end() const
{
	return m_container.end();
}

template <typename T, typename Connection>
inline
typename Reader<T, Connection>::iterator
Reader<T, Connection>::end()
{
	return m_container.end();
}



template <typename T, typename Connection>
bool
Reader<T, Connection>::read()
{
		try
		{
			// Assignment is intentional
			return m_is_valid = m_statement.step();
		}
		catch (SQLiteException&)
		{
			m_is_valid = false;
			throw;
		}
}

template <typename T, typename Connection>
T
Reader<T, Connection>::item() const
{
	if (m_is_valid)
	{
		return T::create_unchecked
		(	m_database_connection, m_statement.extract<Id>(0)
		);
	}
	assert (!m_is_valid);
	throw InvalidReader("Reader is not at a result row.");
}
	

}  // namespace sqloxx


#endif  // GUARD_reader_hpp
