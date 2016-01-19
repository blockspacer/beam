#ifndef BEAM_MYSQLSERVICELOCATORDATASTORE_HPP
#define BEAM_MYSQLSERVICELOCATORDATASTORE_HPP
#include <boost/thread/mutex.hpp>
#include <boost/throw_exception.hpp>
#include "Beam/IO/OpenState.hpp"
#include "Beam/MySql/PosixTimeToMySqlDateTime.hpp"
#include "Beam/Network/IpAddress.hpp"
#include "Beam/ServiceLocator/MySqlServiceLocatorDataStoreDetails.hpp"
#include "Beam/ServiceLocator/ServiceLocatorDataStore.hpp"
#include "Beam/ServiceLocator/ServiceLocatorDataStoreException.hpp"

namespace Beam {
namespace ServiceLocator {

  /*! \class MySqlServiceLocatorDataStore
      \brief Implements the ServiceLocatorDataStore using MySQL.
   */
  class MySqlServiceLocatorDataStore : public ServiceLocatorDataStore {
    public:

      //! Constructs a MySqlServiceLocatorDataStore.
      /*!
        \param address The IP address of the MySQL database to connect to.
        \param schema The name of the schema.
        \param username The username to connect as.
        \param password The password associated with the <i>username</i>.
      */
      MySqlServiceLocatorDataStore(const Network::IpAddress& address,
        const std::string& schema, const std::string& username,
        const std::string& password);

      virtual ~MySqlServiceLocatorDataStore();

      virtual std::vector<DirectoryEntry> LoadParents(
        const DirectoryEntry& entry);

      virtual std::vector<DirectoryEntry> LoadChildren(
        const DirectoryEntry& directory);

      virtual DirectoryEntry LoadDirectoryEntry(unsigned int id);

      virtual std::vector<DirectoryEntry> LoadAllAccounts();

      virtual DirectoryEntry LoadAccount(const std::string& name);

      virtual DirectoryEntry MakeAccount(const std::string& name,
        const std::string& password, const DirectoryEntry& parent,
        const boost::posix_time::ptime& registrationTime);

      virtual DirectoryEntry MakeDirectory(const std::string& name,
        const DirectoryEntry& parent);

      virtual void Delete(const DirectoryEntry& entry);

      virtual bool Associate(const DirectoryEntry& entry,
        const DirectoryEntry& parent);

      virtual bool Detach(const DirectoryEntry& entry,
        const DirectoryEntry& parent);

      virtual std::string LoadPassword(const DirectoryEntry& account);

      virtual void SetPassword(const DirectoryEntry& account,
        const std::string& password);

      virtual Permissions LoadPermissions(const DirectoryEntry& source,
        const DirectoryEntry& target);

      virtual void SetPermissions(const DirectoryEntry& source,
        const DirectoryEntry& target, Permissions permissions);

      virtual boost::posix_time::ptime LoadRegistrationTime(
        const DirectoryEntry& account);

      virtual boost::posix_time::ptime LoadLastLoginTime(
        const DirectoryEntry& account);

      virtual void StoreLastLoginTime(const DirectoryEntry& account,
        const boost::posix_time::ptime& loginTime);

      virtual void WithTransaction(const std::function<void ()>& transaction);

      virtual void Open();

      virtual void Close();

    private:
      mutable boost::mutex m_mutex;
      Network::IpAddress m_address;
      std::string m_schema;
      std::string m_username;
      std::string m_password;
      mysqlpp::Connection m_databaseConnection;
      unsigned int m_nextEntryId;
      IO::OpenState m_openState;

      void Shutdown();
      unsigned int LoadNextEntryId();
  };

  inline MySqlServiceLocatorDataStore::MySqlServiceLocatorDataStore(
      const Network::IpAddress& address, const std::string& schema,
      const std::string& username, const std::string& password)
      : m_address(address),
        m_schema(schema),
        m_username(username),
        m_password(password),
        m_databaseConnection(false) {}

  inline MySqlServiceLocatorDataStore::~MySqlServiceLocatorDataStore() {
    Close();
  }

  inline std::vector<DirectoryEntry> MySqlServiceLocatorDataStore::LoadParents(
      const DirectoryEntry& entry) {
    mysqlpp::Query query = m_databaseConnection.query();
    query << "SELECT parent FROM parents WHERE entry = " << entry.m_id;
    mysqlpp::StoreQueryResult result = query.store();
    if(!result) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    std::vector<DirectoryEntry> parents;
    for(mysqlpp::StoreQueryResult::const_iterator i = result.begin();
        i != result.end(); ++i) {
      parents.push_back(LoadDirectoryEntry(static_cast<int>((*i)[0])));
    }
    return parents;
  }

  inline std::vector<DirectoryEntry> MySqlServiceLocatorDataStore::LoadChildren(
      const DirectoryEntry& entry) {
    mysqlpp::Query query = m_databaseConnection.query();
    query << "SELECT child FROM children WHERE entry = " << entry.m_id;
    mysqlpp::StoreQueryResult result = query.store();
    if(!result) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    std::vector<DirectoryEntry> children;
    for(mysqlpp::StoreQueryResult::const_iterator i = result.begin();
        i != result.end(); ++i) {
      children.push_back(LoadDirectoryEntry(
        static_cast<unsigned int>((*i)[0])));
    }
    return children;
  }

  inline DirectoryEntry MySqlServiceLocatorDataStore::LoadDirectoryEntry(
      unsigned int id) {
    mysqlpp::Query query = m_databaseConnection.query();
    query << "SELECT type, id, name FROM "
      "(SELECT " << DirectoryEntry::Type::ACCOUNT << " AS type, id, name "
      "FROM accounts UNION ALL SELECT " << DirectoryEntry::Type::DIRECTORY <<
      " AS type, id, name FROM directories) AS directory_entries WHERE id = " <<
      id;
    mysqlpp::StoreQueryResult result = query.store();
    if(!result) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    if(result.empty()) {
      BOOST_THROW_EXCEPTION(
        ServiceLocatorDataStoreException("Directory entry not found."));
    }
    assert(result.size() == 1);
    DirectoryEntry entry(static_cast<DirectoryEntry::Type>(
      static_cast<int>(result[0][0])), static_cast<unsigned int>(result[0][1]),
      result[0][2].c_str());
    return entry;
  }

  inline std::vector<DirectoryEntry> MySqlServiceLocatorDataStore::
      LoadAllAccounts() {
    mysqlpp::Query query = m_databaseConnection.query();
    query << "SELECT * FROM accounts";
    mysqlpp::StoreQueryResult result = query.store();
    if(!result) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    std::vector<DirectoryEntry> accounts;
    for(mysqlpp::StoreQueryResult::const_iterator i = result.begin();
        i != result.end(); ++i) {
      accounts.push_back(LoadDirectoryEntry(
        static_cast<unsigned int>((*i)[0])));
    }
    return accounts;
  }

  inline DirectoryEntry MySqlServiceLocatorDataStore::LoadAccount(
      const std::string& name) {
    mysqlpp::Query query = m_databaseConnection.query();
    query << "SELECT id FROM accounts WHERE name = " << mysqlpp::quote << name;
    mysqlpp::StoreQueryResult result = query.store();
    if(!result) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    if(result.empty()) {
      BOOST_THROW_EXCEPTION(
        ServiceLocatorDataStoreException("Account not found."));
    }
    assert(result.size() == 1);
    DirectoryEntry entry(DirectoryEntry::Type::ACCOUNT, result[0][0],
      name);
    return entry;
  }

  inline DirectoryEntry MySqlServiceLocatorDataStore::MakeAccount(
      const std::string& name, const std::string& password,
      const DirectoryEntry& parent,
      const boost::posix_time::ptime& registrationTime) {
    if(parent.m_type != DirectoryEntry::Type::DIRECTORY) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(
        "Parent must be a directory."));
    }
    bool accountExists;
    try {
      DirectoryEntry existingAccount = LoadAccount(name);
      accountExists = true;
    } catch(const ServiceLocatorDataStoreException&) {
      accountExists = false;
    }
    if(accountExists) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(
        "An account with the specified name exists."));
    }
    unsigned int entryId = LoadNextEntryId();
    DirectoryEntry newEntry(DirectoryEntry::Type::ACCOUNT, entryId, name);
    mysqlpp::Query query = m_databaseConnection.query();
    Details::SqlInsert::accounts accountRow(newEntry.m_id, newEntry.m_name,
      HashPassword(newEntry, password), MySql::ToDateTime(registrationTime),
      MySql::ToDateTime(boost::posix_time::neg_infin));
    query.insert(accountRow);
    if(!query.execute()) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    Associate(newEntry, parent);
    return newEntry;
  }

  inline DirectoryEntry MySqlServiceLocatorDataStore::MakeDirectory(
      const std::string& name, const DirectoryEntry& parent) {
    if(parent.m_type != DirectoryEntry::Type::DIRECTORY) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(
        "Parent must be a directory."));
    }
    bool accountExists;
    try {
      DirectoryEntry existingAccount = LoadAccount(name);
      accountExists = true;
    } catch(const ServiceLocatorDataStoreException&) {
      accountExists = false;
    }
    if(accountExists) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(
        "An account with the specified name exists."));
    }
    unsigned int entryId = LoadNextEntryId();
    DirectoryEntry newEntry(DirectoryEntry::Type::DIRECTORY, entryId, name);
    mysqlpp::Query query = m_databaseConnection.query();
    Details::SqlInsert::directories directoryRow(newEntry.m_id,
      newEntry.m_name);
    query.insert(directoryRow);
    if(!query.execute()) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    Associate(newEntry, parent);
    return newEntry;
  }

  inline void MySqlServiceLocatorDataStore::Delete(
      const DirectoryEntry& entry) {
    std::vector<DirectoryEntry> children = LoadChildren(entry);
    if(!children.empty()) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(
        "Directory entry is not empty."));
    }
    mysqlpp::Query query = m_databaseConnection.query();
    query << "DELETE FROM permissions WHERE source = " << entry.m_id <<
      " OR " << " target = " << entry.m_id;
    if(!query.execute()) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    query.reset();
    std::vector<DirectoryEntry> parents = LoadParents(entry);
    for(const DirectoryEntry& parent : parents) {
      Detach(entry, parent);
    }
    if(entry.m_type == DirectoryEntry::Type::ACCOUNT) {
      query << "DELETE FROM accounts WHERE id = " << entry.m_id;
      if(!query.execute()) {
        BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
      }
      query.reset();
    } else {
      query << "DELETE FROM directories WHERE id = " << entry.m_id;
      if(!query.execute()) {
        BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
      }
      query.reset();
    }
  }

  inline bool MySqlServiceLocatorDataStore::Associate(
      const DirectoryEntry& entry, const DirectoryEntry& parent) {
    if(parent.m_id == -1) {
      return false;
    }
    if(parent.m_type != DirectoryEntry::Type::DIRECTORY) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(
        "Parent not found."));
    }
    if(entry == parent) {
      return false;
    }
    std::vector<DirectoryEntry> parents = LoadParents(entry);
    if(std::find(parents.begin(), parents.end(), parent) != parents.end()) {
      return false;
    }
    mysqlpp::Query query = m_databaseConnection.query();
    Details::SqlInsert::parents parentsRow(entry.m_id, parent.m_id);
    query.insert(parentsRow);
    if(!query.execute()) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    query.reset();
    Details::SqlInsert::children childrenRow(parent.m_id, entry.m_id);
    query.insert(childrenRow);
    if(!query.execute()) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    return true;
  }

  inline bool MySqlServiceLocatorDataStore::Detach(
      const DirectoryEntry& entry, const DirectoryEntry& parent) {
    if(parent.m_type != DirectoryEntry::Type::DIRECTORY) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(
        "Parent not found."));
    }
    if(entry == parent) {
      return false;
    }
    mysqlpp::Query query = m_databaseConnection.query();
    query << "DELETE FROM parents WHERE entry = " << entry.m_id <<
      " AND parent = " << parent.m_id;
    if(!query.execute()) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    query.reset();
    query << "DELETE FROM children WHERE entry = " << parent.m_id <<
      " AND child = " << entry.m_id;
    if(!query.execute()) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    return query.affected_rows() > 0;
  }

  inline std::string MySqlServiceLocatorDataStore::LoadPassword(
      const DirectoryEntry& account) {
    if(account.m_type != DirectoryEntry::Type::ACCOUNT) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(
        "Account not found."));
    }
    mysqlpp::Query query = m_databaseConnection.query();
    query << "SELECT password FROM accounts WHERE id = " << account.m_id;
    mysqlpp::StoreQueryResult result = query.store();
    if(!result) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    assert(result.size() == 1);
    return result[0][0].c_str();
  }

  inline void MySqlServiceLocatorDataStore::SetPassword(
      const DirectoryEntry& account, const std::string& password) {
    mysqlpp::Query query = m_databaseConnection.query();
    query << "UPDATE accounts SET password = " << mysqlpp::quote << password <<
      " WHERE id = " << account.m_id;
    if(!query.execute()) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
  }

  inline Permissions MySqlServiceLocatorDataStore::LoadPermissions(
      const DirectoryEntry& source, const DirectoryEntry& target) {
    mysqlpp::Query query = m_databaseConnection.query();
    query << "SELECT permission FROM permissions WHERE source = " <<
      source.m_id << " AND target = " << target.m_id;
    mysqlpp::StoreQueryResult result = query.store();
    if(!result) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    if(result.empty()) {
      return Permission::NONE;
    }
    return Permissions::FromRepresentation(result[0][0].conv<int>(0));
  }

  inline void MySqlServiceLocatorDataStore::SetPermissions(
      const DirectoryEntry& source, const DirectoryEntry& target,
      Permissions permissions) {
    mysqlpp::Query query = m_databaseConnection.query();
    query << "SELECT * FROM permissions WHERE source = " << source.m_id <<
      " AND target = " << target.m_id;
    mysqlpp::StoreQueryResult result = query.store();
    if(!result) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    query.reset();
    if(result.empty()) {
      Details::SqlInsert::permissions permissionRow(source.m_id, target.m_id,
        permissions.GetBitset().to_ulong());
      query.insert(permissionRow);
    } else {
      query << "UPDATE permissions SET permission = " <<
        permissions.GetBitset().to_ulong() << " WHERE source = " <<
        source.m_id << " AND target = " << target.m_id;
    }
    if(!query.execute()) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
  }

  inline boost::posix_time::ptime MySqlServiceLocatorDataStore::
      LoadRegistrationTime(const DirectoryEntry& account) {
    if(account.m_type != DirectoryEntry::Type::ACCOUNT) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(
        "Account not found."));
    }
    mysqlpp::Query query = m_databaseConnection.query();
    query << "SELECT registration_time FROM accounts WHERE id = " <<
      account.m_id;
    mysqlpp::StoreQueryResult result = query.store();
    if(!result) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    assert(result.size() == 1);
    return MySql::FromDateTime(static_cast<mysqlpp::DateTime>(result[0][0]));
  }

  inline boost::posix_time::ptime MySqlServiceLocatorDataStore::
      LoadLastLoginTime(const DirectoryEntry& account) {
    if(account.m_type != DirectoryEntry::Type::ACCOUNT) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(
        "Account not found."));
    }
    mysqlpp::Query query = m_databaseConnection.query();
    query << "SELECT last_login_time FROM accounts WHERE id = " << account.m_id;
    mysqlpp::StoreQueryResult result = query.store();
    if(!result) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    assert(result.size() == 1);
    return MySql::FromDateTime(static_cast<mysqlpp::DateTime>(result[0][0]));
  }

  inline void MySqlServiceLocatorDataStore::StoreLastLoginTime(
      const DirectoryEntry& account,
      const boost::posix_time::ptime& loginTime) {
    if(account.m_type != DirectoryEntry::Type::ACCOUNT) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(
        "Account not found."));
    }
    mysqlpp::Query query = m_databaseConnection.query();
    query << "UPDATE accounts SET last_login_time = " << mysqlpp::quote <<
      MySql::ToDateTime(loginTime) << " WHERE id = " << account.m_id;
    if(!query.execute()) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
  }

  inline void MySqlServiceLocatorDataStore::WithTransaction(
      const std::function<void ()>& transaction) {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    mysqlpp::Transaction t(m_databaseConnection);
    try {
      transaction();
    } catch(...) {
      t.rollback();
      throw;
    }
    t.commit();
  }

  inline void MySqlServiceLocatorDataStore::Open() {
    if(m_openState.SetOpening()) {
      return;
    }
    try {
      bool connectionResult = m_databaseConnection.set_option(
        new mysqlpp::ReconnectOption(true));
      if(!connectionResult) {
        BOOST_THROW_EXCEPTION(
          IO::IOException("Unable to set MySQL reconnect option."));
        return;
      }
      connectionResult = m_databaseConnection.connect(m_schema.c_str(),
        m_address.GetHost().c_str(), m_username.c_str(), m_password.c_str(),
        m_address.GetPort());
      if(!connectionResult) {
        BOOST_THROW_EXCEPTION(IO::ConnectException(std::string(
          "Unable to connect to MySQL database - ") +
          m_databaseConnection.error()));
      }
      if(!Details::LoadTables(m_databaseConnection, m_schema)) {
        BOOST_THROW_EXCEPTION(
          IO::IOException("Unable to load database tables."));
      }
      if(!Details::LoadSettings(m_databaseConnection, Store(m_nextEntryId))) {
        BOOST_THROW_EXCEPTION(IO::IOException(
          "Unable to load database settings."));
      }
    } catch(const std::exception&) {
      m_openState.SetOpenFailure();
      Shutdown();
    }
    m_openState.SetOpen();
  }

  inline void MySqlServiceLocatorDataStore::Close() {
    if(m_openState.SetClosing()) {
      return;
    }
    Shutdown();
  }

  inline void MySqlServiceLocatorDataStore::Shutdown() {
    m_databaseConnection.disconnect();
    m_openState.SetClosed();
  }

  inline unsigned int MySqlServiceLocatorDataStore::LoadNextEntryId() {
    unsigned int result = m_nextEntryId;
    mysqlpp::Query query = m_databaseConnection.query();
    query << "UPDATE settings SET next_entry_id = " << (m_nextEntryId + 1);
    if(!query.execute()) {
      BOOST_THROW_EXCEPTION(ServiceLocatorDataStoreException(query.error()));
    }
    ++m_nextEntryId;
    return result;
  }
}
}

#endif
