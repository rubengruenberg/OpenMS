// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2018.
//
// This software is released under a three-clause BSD license:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of any author or any participating institution
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// For a full list of authors, refer to the file AUTHORS.
// --------------------------------------------------------------------------
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL ANY OF THE AUTHORS OR THE CONTRIBUTING
// INSTITUTIONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// --------------------------------------------------------------------------
// $Maintainer: Hendrik Weisser $
// $Authors: Julianus Pfeuffer, Oliver Alka, Hendrik Weisser $
// --------------------------------------------------------------------------

#include <OpenMS/FORMAT/OMSFile.h>
#include <OpenMS/SYSTEM/File.h>
#include <OpenMS/CONCEPT/VersionInfo.h>

// strangely, this is needed for type conversions in "QSqlQuery::bindValue":
#include <QtSql/QSqlQueryModel>

using namespace std;

using ID = OpenMS::IdentificationData;

namespace OpenMS
{
  int version_number = 1;

  void OMSFile::raiseDBError_(const QSqlError& error, int line,
                              const char* function, const String& context)
  {
    // clean-up:
    db_.close();
    QSqlDatabase::removeDatabase(db_.connectionName());

    String msg = context + ": " + error.text();
    throw Exception::FailedAPICall(__FILE__, line, function, msg);
  }


  bool OMSFile::tableExists_(const String& name) const
  {
    return db_.tables(QSql::Tables).contains(name.toQString());
  }


  void OMSFile::createTable_(const String& name, const String& definition,
                             bool may_exist)
  {
    QString sql_create = "CREATE TABLE ";
    if (may_exist) sql_create += "IF NOT EXISTS ";
    sql_create += name.toQString() + " (" + definition.toQString() + ")";
    QSqlQuery query(db_);
    if (!query.exec(sql_create))
    {
      String msg = "error creating database table " + name;
      raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION, msg);
    }
  }


  void OMSFile::storeVersionAndDate_()
  {
    createTable_("version",
                 "OMSFile INT NOT NULL, "       \
                 "date TEXT NOT NULL, "         \
                 "OpenMS TEXT, "                \
                 "build_date TEXT");

    QSqlQuery query(db_);
    query.prepare("INSERT INTO version VALUES ("  \
                  ":format_version, "             \
                  "datetime('now'), "             \
                  ":openms_version, "             \
                  ":build_date)");
    query.bindValue(":format_version", version_number);
    query.bindValue(":openms_version", VersionInfo::getVersion().toQString());
    query.bindValue(":build_date", VersionInfo::getTime().toQString());
    if (!query.exec())
    {
      raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                    "error inserting data");
    }
  }


  void OMSFile::createTableDataValue_()
  {
    createTable_("DataValue_DataType",
                 "id INTEGER PRIMARY KEY NOT NULL, "  \
                 "data_type TEXT UNIQUE NOT NULL");
    QString sql_insert =
      "INSERT INTO DataValue_DataType VALUES " \
      "(1, 'STRING_VALUE'), "                  \
      "(2, 'INT_VALUE'), "                     \
      "(3, 'DOUBLE_VALUE'), "                  \
      "(4, 'STRING_LIST'), "                   \
      "(5, 'INT_LIST'), "                      \
      "(6, 'DOUBLE_LIST')";
    QSqlQuery query(db_);
    if (!query.exec(sql_insert))
    {
      raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                    "error inserting data");
    }
    createTable_(
      "DataValue",
      "id INTEGER PRIMARY KEY NOT NULL, "                               \
      "data_type_id INTEGER, "                                          \
      "value TEXT, "                                                    \
      "FOREIGN KEY (data_type_id) REFERENCES DataValue_DataType (id)");
    // @TODO: add support for units
  }


  OMSFile::Key OMSFile::storeDataValue_(const DataValue& value)
  {
    // this assumes the "DataValue" table exists already!
    // @TODO: split this up and make several tables for different types?
    QSqlQuery query(db_);
    query.prepare("INSERT INTO DataValue VALUES (" \
                  "NULL, "                         \
                  ":data_type, "                   \
                  ":value)");
    // @TODO: cache the prepared query between function calls somehow?
    if (!value.isEmpty()) // use NULL as the type for empty values
    {
      query.bindValue(":data_type", int(value.valueType()) + 1);
    }
    query.bindValue(":value", value.toQString());
    if (!query.exec())
    {
      raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                    "error inserting data");
    }
    return query.lastInsertId().toInt();
  }


  void OMSFile::createTableCVTerm_()
  {
    createTable_("CVTerm",
                 "id INTEGER PRIMARY KEY NOT NULL, "        \
                 "accession TEXT UNIQUE, "                  \
                 "name TEXT NOT NULL, "                     \
                 "cv_identifier_ref TEXT, "                 \
                 // does this constrain "name" if "accession" is NULL?
                 "UNIQUE (accession, name)");
    // @TODO: add support for unit and value
  }


  OMSFile::Key OMSFile::storeCVTerm_(const CVTerm& cv_term)
  {
    // this assumes the "CVTerm" table exists already!
    QSqlQuery query(db_);
    query.prepare("INSERT OR IGNORE INTO CVTerm VALUES ("   \
                  "NULL, "                                  \
                  ":accession, "                            \
                  ":name, "                                 \
                  ":cv_identifier_ref)");
    if (!cv_term.getAccession().empty()) // use NULL for empty accessions
    {
      query.bindValue(":accession", cv_term.getAccession().toQString());
    }
    query.bindValue(":name", cv_term.getName().toQString());
    query.bindValue(":cv_identifier_ref",
                    cv_term.getCVIdentifierRef().toQString());
    if (!query.exec())
    {
      raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                    "error updating database");
    }
    if (query.lastInsertId().isValid()) return query.lastInsertId().toInt();
    // else: insert has failed, record must already exist - get the key:
    query.prepare("SELECT id FROM CVTerm "                          \
                  "WHERE accession = :accession AND name = :name");
    if (!cv_term.getAccession().empty()) // use NULL for empty accessions
    {
      query.bindValue(":accession", cv_term.getAccession().toQString());
    }
    query.bindValue(":name", cv_term.getName().toQString());
    if (!query.exec() || !query.next())
    {
      raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                    "error querying database");
    }
    return query.value(0).toInt();
  }


  void OMSFile::createTableMetaInfo_(const String& parent_table)
  {
    if (!tableExists_("DataValue")) createTableDataValue_();

    createTable_(
      parent_table + "_MetaInfo",
      "parent_id INTEGER PRIMARY KEY NOT NULL, "                        \
      "name TEXT NOT NULL, "                                            \
      "data_value_id INTEGER NOT NULL, "                                \
      "FOREIGN KEY (parent_id) REFERENCES " + parent_table + " (id), "  \
      "FOREIGN KEY (data_value_id) REFERENCES DataValue (id), "         \
      "UNIQUE (parent_id, name)");
  }


  void OMSFile::storeMetaInfo_(const MetaInfoInterface& info,
                               const String& parent_table, Key parent_id)
  {
    if (info.isMetaEmpty()) return;

    // this assumes the "..._MetaInfo" and "DataValue" tables exists already!
    String table = parent_table + "_MetaInfo";

    QSqlQuery query(db_);
    query.prepare("INSERT INTO " + table.toQString() + " VALUES ("  \
                  ":parent_id, "                                    \
                  ":name, "                                         \
                  ":data_value_id)");
    query.bindValue(":parent_id", parent_id);
    // this is inefficient, but MetaInfoInterface doesn't support iteration:
    vector<String> info_keys;
    info.getKeys(info_keys);
    for (const String& info_key : info_keys)
    {
      query.bindValue(":name", info_key.toQString());
      Key value_id = storeDataValue_(info.getMetaValue(info_key));
      query.bindValue(":data_value_id", value_id);
      if (!query.exec())
      {
        raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                      "error inserting data");
      }
    }
  }


  void OMSFile::storeScoreTypes_(const IdentificationData& id_data)
  {
    if (id_data.getScoreTypes().empty()) return;

    createTableCVTerm_();
    createTable_(
      "ID_ScoreType",
      "id INTEGER PRIMARY KEY NOT NULL, "                               \
      "cv_term_id INTEGER NOT NULL, "                                   \
      "higher_better NUMERIC NOT NULL CHECK (higher_better in (0, 1)), " \
      "FOREIGN KEY (cv_term_id) REFERENCES CVTerm (id)");

    QSqlQuery query(db_);
    query.prepare("INSERT INTO ID_ScoreType VALUES ("       \
                  ":id, "                                   \
                  ":cv_term_id, "                           \
                  ":higher_better)");
    for (const ID::ScoreType& score_type : id_data.getScoreTypes())
    {
      Key cv_id = storeCVTerm_(score_type.cv_term);
      query.bindValue(":id", Key(&score_type)); // use address as primary key
      query.bindValue(":cv_term_id", cv_id);
      query.bindValue(":higher_better", int(score_type.higher_better));
      if (!query.exec())
      {
        raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                      "error inserting data");
      }
    }
  }


  void OMSFile::storeInputFiles_(const IdentificationData& id_data)
  {
    if (id_data.getInputFiles().empty()) return;

    createTable_("ID_InputFile",
                 "id INTEGER PRIMARY KEY NOT NULL, "  \
                 "file TEXT UNIQUE NOT NULL");

    QSqlQuery query(db_);
    query.prepare("INSERT INTO ID_InputFile VALUES ("  \
                  ":id, "                              \
                  ":file)");
    for (const String& input_file : id_data.getInputFiles())
    {
      query.bindValue(":id", Key(&input_file));
      query.bindValue(":file", input_file.toQString());
      if (!query.exec())
      {
        raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                      "error inserting data");
      }
    }
  }


  void OMSFile::storeDataProcessingSoftwares_(const IdentificationData& id_data)
  {
    if (id_data.getDataProcessingSoftwares().empty()) return;

    createTable_("ID_DataProcessingSoftware",
                 "id INTEGER PRIMARY KEY NOT NULL, "  \
                 "name TEXT NOT NULL, "               \
                 "version TEXT, "                     \
                 "UNIQUE (name, version)");

    QSqlQuery query(db_);
    query.prepare("INSERT INTO ID_DataProcessingSoftware VALUES ("  \
                  ":id, "                                           \
                  ":name, "                                         \
                  ":version)");
    bool any_scores = false; // does any software have assigned scores stored?
    for (const ID::DataProcessingSoftware& software :
           id_data.getDataProcessingSoftwares())
    {
      if (!software.assigned_scores.empty()) any_scores = true;
      query.bindValue(":id", Key(&software));
      query.bindValue(":name", software.getName().toQString());
      query.bindValue(":version", software.getVersion().toQString());
      if (!query.exec())
      {
        raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                      "error inserting data");
      }
    }
    if (any_scores)
    {
      createTable_(
        "ID_DataProcessingSoftware_AssignedScore",
        "software_id INTEGER NOT NULL, "                                \
        "score_type_id INTEGER NOT NULL, "                              \
        "UNIQUE (software_id, score_type_id), "                         \
        "FOREIGN KEY (software_id) REFERENCES ID_DataProcessingSoftware (id), " \
        "FOREIGN KEY (score_type_id) REFERENCES ID_ScoreType (id)");

      query.prepare(
        "INSERT INTO ID_DataProcessingSoftware_AssignedScore VALUES ("  \
        ":software_id, "                                                \
        ":score_type_id)");
      for (const ID::DataProcessingSoftware& software :
             id_data.getDataProcessingSoftwares())
      {
        query.bindValue(":software_id", Key(&software));
        for (ID::ScoreTypeRef score_type_ref : software.assigned_scores)
        {
          query.bindValue(":score_type_id", Key(&(*score_type_ref)));
          if (!query.exec())
          {
            raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                          "error inserting data");
          }
        }
      }
    }
  }


  void OMSFile::storeDataProcessingSteps_(const IdentificationData& id_data)
  {
    if (id_data.getDataProcessingSteps().empty()) return;

    createTable_("ID_DataProcessingStep",
                 "id INTEGER PRIMARY KEY NOT NULL, " \
                 "software_id INTEGER NOT NULL, "    \
                 "primary_files TEXT, "              \
                 "date_time TEXT");
    // @TODO: add support for processing actions
    // @TODO: store primary files in a separate table (like input files)?

    QSqlQuery query(db_);
    query.prepare("INSERT INTO ID_DataProcessingStep VALUES ("  \
                  ":id, "                                       \
                  ":software_id, "                              \
                  ":primary_files, "                            \
                  ":data_time)");
    bool any_input_files = false, any_meta_values = false;
    for (const ID::DataProcessingStep& step : id_data.getDataProcessingSteps())
    {
      if (!step.input_file_refs.empty()) any_input_files = true;
      if (!step.isMetaEmpty()) any_meta_values = true;
      query.bindValue(":id", Key(&step));
      query.bindValue(":software_id", Key(&(*step.software_ref)));
      // @TODO: what if a primary file name contains ","?
      String primary_files = ListUtils::concatenate(step.primary_files, ",");
      query.bindValue(":primary_files", primary_files.toQString());
      query.bindValue(":date_time", step.date_time.get().toQString());
      if (!query.exec())
      {
        raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                      "error inserting data");
      }
    }
    if (any_input_files)
    {
      createTable_(
        "ID_DataProcessingStep_InputFile",
        "processing_step_id INTEGER NOT NULL, "                         \
        "input_file_id INTEGER NOT NULL, "                              \
        "FOREIGN KEY processing_step_id REFERENCES ID_DataProcessingStep (id), " \
        "FOREIGN KEY input_file_id REFERENCES ID_InputFile (id), "      \
        "UNIQUE (processing_step_id, input_file_id)");

      query.prepare("INSERT INTO ID_DataProcessingStep_InputFile VALUES (" \
                    ":processing_step_id, "                             \
                    ":input_file_id)");

      for (const ID::DataProcessingStep& step :
             id_data.getDataProcessingSteps())
      {
        query.bindValue(":processing_step_id", Key(&step));
        for (ID::InputFileRef input_file_ref : step.input_file_refs)
        {
          query.bindValue(":input_file_id", Key(&(*input_file_ref)));
          if (!query.exec())
          {
            raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                          "error inserting data");
          }
        }
      }
    }
    if (any_meta_values)
    {
      createTableMetaInfo_("ID_DataProcessingStep");

      for (const ID::DataProcessingStep& step :
             id_data.getDataProcessingSteps())
      {
        storeMetaInfo_(step, "ID_DataProcessingStep", Key(&step));
      }
    }
  }


  void OMSFile::storeParentMolecules_(const IdentificationData& id_data)
  {
    if (id_data.getParentMolecules().empty()) return;

    // table for molecule types (enum MoleculeType):
    createTable_("ID_MoleculeType",
                 "id INTEGER PRIMARY KEY NOT NULL, "    \
                 "molecule_type TEXT UNIQUE NOT NULL");

    QString sql_insert =
      "INSERT INTO ID_MoleculeType VALUES "     \
      "(1, 'PROTEIN'), "                        \
      "(2, 'COMPOUND'), "                       \
      "(3, 'RNA')";
    QSqlQuery query(db_);
    if (!query.exec(sql_insert))
    {
      raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                    "error inserting data");
    }

    createTable_(
      "ID_ParentMolecule",
      "id INTEGER PRIMARY KEY NOT NULL, "                               \
      "accession TEXT UNIQUE NOT NULL, "                                \
      "molecule_type_id INTEGER NOT NULL, "                             \
      "sequence TEXT, "                                                 \
      "description TEXT, "                                              \
      "coverage REAL, "                                                 \
      "is_decoy NUMERIC NOT NULL CHECK (is_decoy in (0, 1)) DEFAULT 0, " \
      "FOREIGN KEY (molecule_type_id) REFERENCES ID_MoleculeType (id)");

    query.prepare("INSERT INTO ID_ParentMolecule VALUES ("  \
                  ":id, "                                   \
                  ":accession, "                            \
                  ":molecule_type, "                        \
                  ":sequence, "                             \
                  ":description, "                          \
                  ":coverage, "                             \
                  ":is_decoy)");
    for (const ID::ParentMolecule& parent : id_data.getParentMolecules())
    {
      query.bindValue(":id", Key(&parent)); // use address as primary key
      query.bindValue(":accession", parent.accession.toQString());
      query.bindValue(":molecule_type", int(parent.molecule_type) + 1);
      query.bindValue(":sequence", parent.sequence.toQString());
      query.bindValue(":description", parent.description.toQString());
      query.bindValue(":coverage", parent.coverage);
      query.bindValue(":is_decoy", int(parent.is_decoy));
      if (!query.exec())
      {
        raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                      "error inserting data");
      }
    }
  }


  void OMSFile::store(const String& filename, const IdentificationData& id_data)
  {
    // delete output file if present:
    File::remove(filename);

    // open database:
    QString connection = "store_" + filename.toQString();
    db_ = QSqlDatabase::addDatabase("QSQLITE", connection);
    db_.setDatabaseName(filename.toQString());
    if (!db_.open())
    {
      raiseDBError_(db_.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                    "error opening SQLite database");
    }
    // generally, create tables only if we have data to write - no empty ones!

    storeVersionAndDate_();
    storeInputFiles_(id_data);
    storeScoreTypes_(id_data);
    storeDataProcessingSoftwares_(id_data);
    storeDataProcessingSteps_(id_data);
    storeParentMolecules_(id_data);

    // this currently produces a warning (because "db_" is still in scope):
    QSqlDatabase::removeDatabase(connection);
  }


  // currently not needed:
  // CVTerm OMSFile::loadCVTerm_(int id)
  // {
  //   // this assumes that the "CVTerm" table exists!
  //   QSqlQuery query(db_);
  //   query.setForwardOnly(true);
  //   QString sql_select = "SELECT * FROM CVTerm WHERE id = " + QString(id);
  //   if (!query.exec(sql_select) || !query.next())
  //   {
  //     raiseDBError_(model.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
  //                   "error reading from database");
  //   }
  //   return CVTerm(query.value("accession").toString(),
  //                 query.value("name").toString(),
  //                 query.value("cv_identifier_ref").toString());
  // }


  void OMSFile::loadScoreTypes_(IdentificationData& id_data)
  {
    score_type_refs_.clear();
    if (!tableExists_("ID_ScoreType")) return;
    if (!tableExists_("CVTerm")) // every score type is a CV term
    {
      String msg = "required database table 'CVTerm' not found";
      throw Exception::MissingInformation(__FILE__, __LINE__,
                                          OPENMS_PRETTY_FUNCTION, msg);
    }
    QSqlQuery query(db_);
    query.setForwardOnly(true);
    if (!query.exec("SELECT * FROM ID_ScoreType JOIN CVTerm "   \
                    "ON ID_ScoreType.cv_term_id = CVTerm.id"))
    {
      raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                    "error reading from database");
    }
    while (query.next())
    {
      CVTerm cv_term(query.value("accession").toString(),
                     query.value("name").toString(),
                     query.value("cv_identifier_ref").toString());
      bool higher_better = query.value("higher_better").toInt();
      ID::ScoreType score_type(cv_term, higher_better);
      ID::ScoreTypeRef ref = id_data.registerScoreType(score_type);
      score_type_refs_[query.value("id").toInt()] = ref;
    }
  }


  void OMSFile::loadInputFiles_(IdentificationData& id_data)
  {
    if (!tableExists_("ID_InputFile")) return;

    QSqlQuery query(db_);
    query.setForwardOnly(true);
    if (!query.exec("SELECT * FROM ID_InputFile"))
    {
      raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                    "error reading from database");
    }
    while (query.next())
    {
      ID::InputFileRef ref =
        id_data.registerInputFile(query.value("file").toString());
      input_file_refs_[query.value("id").toInt()] = ref;
    }
  }


  void OMSFile::loadDataProcessingSoftwares_(IdentificationData& id_data)
  {
    if (!tableExists_("ID_DataProcessingSoftware")) return;

    QSqlQuery query(db_);
    query.setForwardOnly(true);
    if (!query.exec("SELECT * FROM ID_DataProcessingSoftware"))
    {
      raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                    "error reading from database");
    }
    bool have_scores = tableExists_("ID_DataProcessingSoftware_AssignedScore");
    QSqlQuery subquery(db_);
    if (have_scores)
    {
      subquery.setForwardOnly(true);
      subquery.prepare("SELECT score_type_id "                         \
                       "FROM ID_DataProcessingSoftware_AssignedScore " \
                       "WHERE software_id = :id");
    }
    while (query.next())
    {
      Key id = query.value("id").toInt();
      ID::DataProcessingSoftware software(query.value("name").toString(),
                                          query.value("version").toString());
      if (have_scores)
      {
        subquery.bindValue(":id", id);
        if (!subquery.exec())
        {
          raiseDBError_(subquery.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                        "error reading from database");
        }
        while (subquery.next())
        {
          Key score_type_id = query.value(0).toInt();
          // the foreign key constraint should ensure that look-up succeeds:
          software.assigned_scores.push_back(score_type_refs_[score_type_id]);
        }
        // order in the vector should be the same as in the table:
        reverse(software.assigned_scores.begin(),
                software.assigned_scores.end());
      }
      ID::ProcessingSoftwareRef ref =
        id_data.registerDataProcessingSoftware(software);
      processing_software_refs_[query.value("id").toInt()] = ref;
    }
  }


  DataValue OMSFile::makeDataValue_(const QSqlQuery& query)
  {
    DataValue::DataType type = DataValue::EMPTY_VALUE;
    int type_index = query.value("data_type_id").toInt();
    if (type_index > 0) type = DataValue::DataType(type_index - 1);
    String value = query.value("value").toString();
    switch (type)
    {
    case DataValue::STRING_VALUE:
      return DataValue(value);
    case DataValue::INT_VALUE:
      return DataValue(value.toInt());
    case DataValue::DOUBLE_VALUE:
      return DataValue(value.toDouble());
    case DataValue::STRING_LIST:
      return DataValue(ListUtils::create<String>(value));
    case DataValue::INT_LIST:
      return DataValue(ListUtils::create<int>(value));
    case DataValue::DOUBLE_LIST:
      return DataValue(ListUtils::create<double>(value));
    default: // DataValue::EMPTY_VALUE (avoid warning about missing return)
      return DataValue();
    }
  }


  void OMSFile::loadDataProcessingSteps_(IdentificationData& id_data)
  {
    if (!tableExists_("ID_DataProcessingStep")) return;

    QSqlQuery query(db_);
    query.setForwardOnly(true);
    if (!query.exec("SELECT * FROM ID_DataProcessingStep"))
    {
      raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                    "error reading from database");
    }
    bool have_input_files = tableExists_("ID_DataProcessingStep_InputFile");
    bool have_meta_info = tableExists_("ID_DataProcessingStep_MetaInfo");
    QSqlQuery subquery_file(db_);
    if (have_input_files)
    {
      subquery_file.setForwardOnly(true);
      subquery_file.prepare("SELECT input_file_id "                 \
                            "FROM ID_DataProcessingStep_InputFile " \
                            "WHERE processing_step_id = :id");
    }
    QSqlQuery subquery_info(db_);
    if (have_meta_info)
    {
      subquery_info.setForwardOnly(true);
      subquery_info.prepare(
        "SELECT * FROM ID_DataProcessingStep_MetaInfo AS MI " \
        "JOIN DataValue AS DV ON MI.data_value_id = DV.id "   \
        "WHERE MI.parent_id = :id");
    }
    while (query.next())
    {
      Key id = query.value("id").toInt();
      Key software_id = query.value("software_id").toInt();
      ID::DataProcessingStep step(processing_software_refs_[software_id]);
      String primary_files = query.value("primary_files").toString();
      step.primary_files = ListUtils::create<String>(primary_files);
      String date_time = query.value("date_time").toString();
      if (!date_time.empty()) step.date_time.set(date_time);
      if (have_input_files)
      {
        subquery_file.bindValue(":id", id);
        if (!subquery_file.exec())
        {
          raiseDBError_(subquery_file.lastError(), __LINE__,
                        OPENMS_PRETTY_FUNCTION, "error reading from database");
        }
        while (subquery_file.next())
        {
          Key input_file_id = subquery_file.value(0).toInt();
          // the foreign key constraint should ensure that look-up succeeds:
          step.input_file_refs.push_back(input_file_refs_[input_file_id]);
        }
        // order in the vector should be the same as in the table:
        reverse(step.input_file_refs.begin(), step.input_file_refs.end());
      }
      if (have_meta_info)
      {
        subquery_info.bindValue(":id", id);
        if (!subquery_info.exec())
        {
          raiseDBError_(subquery_info.lastError(), __LINE__,
                        OPENMS_PRETTY_FUNCTION, "error reading from database");
        }
        while (subquery_info.next())
        {
          DataValue value = makeDataValue_(subquery_info);
          step.setMetaValue(subquery_info.value("name").toString(), value);
        }
      }
      id_data.registerDataProcessingStep(step);
    }
  }



  void OMSFile::loadParentMolecules_(IdentificationData& id_data)
  {
    if (!tableExists_("ID_ParentMolecule")) return;

    QSqlQuery query(db_);
    query.setForwardOnly(true);
    if (!query.exec("SELECT * FROM ID_ParentMolecule"))
    {
      raiseDBError_(query.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                    "error reading from database");
    }
    while (query.next())
    {
      String accession = query.value("accession").toString();
      ID::ParentMolecule parent(accession);
      int molecule_type_index = query.value("molecule_type_id").toInt() - 1;
      parent.molecule_type = ID::MoleculeType(molecule_type_index);
      parent.sequence = query.value("sequence").toString();
      parent.description = query.value("description").toString();
      parent.coverage = query.value("coverage").toDouble();
      parent.is_decoy = query.value("is_decoy").toInt();
      id_data.registerParentMolecule(parent);
    }
  }


  void OMSFile::load(const String& filename, IdentificationData& id_data)
  {
    // open database:
    QString connection = "load_" + filename.toQString();
    db_ = QSqlDatabase::addDatabase("QSQLITE", connection);
    db_.setDatabaseName(filename.toQString());
    db_.setConnectOptions("QSQLITE_OPEN_READONLY");
    if (!db_.open())
    {
      raiseDBError_(db_.lastError(), __LINE__, OPENMS_PRETTY_FUNCTION,
                    "error opening SQLite database");
    }

    loadInputFiles_(id_data);
    loadScoreTypes_(id_data);
    loadDataProcessingSoftwares_(id_data);
    loadDataProcessingSteps_(id_data);
    loadParentMolecules_(id_data);

    // this currently produces a warning (because "db_" is still in scope):
    QSqlDatabase::removeDatabase(connection);
  }

}