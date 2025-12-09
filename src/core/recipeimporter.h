/**
 * @file recipeimporter.h
 * @brief Import recipes from JSON file
 */

#ifndef RECIPEIMPORTER_H
#define RECIPEIMPORTER_H

#include <QString>

namespace Frontier {
class Database;
}

class RecipeImporter
{
public:
    explicit RecipeImporter(Frontier::Database *database);

    bool importFromJson(const QString &filePath);

    int workbenchesImported() const { return m_workbenchesImported; }
    int recipesImported() const { return m_recipesImported; }
    QString lastError() const { return m_lastError; }

private:
    Frontier::Database *m_database;
    int m_workbenchesImported = 0;
    int m_recipesImported = 0;
    QString m_lastError;
};

#endif // RECIPEIMPORTER_H
