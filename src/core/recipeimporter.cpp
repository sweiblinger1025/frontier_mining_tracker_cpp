/**
 * @file recipeimporter.cpp
 * @brief Import recipes from JSON file
 */

#include "recipeimporter.h"
#include "database.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

RecipeImporter::RecipeImporter(Frontier::Database *database)
    : m_database(database)
{
}

bool RecipeImporter::importFromJson(const QString &filePath)
{
    m_workbenchesImported = 0;
    m_recipesImported = 0;
    m_lastError.clear();

    // Open and read file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = QString("Cannot open file: %1").arg(file.errorString());
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    // Parse JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        m_lastError = QString("JSON parse error: %1").arg(parseError.errorString());
        return false;
    }

    if (!doc.isObject()) {
        m_lastError = "Expected JSON object with workbench names as keys";
        return false;
    }

    QJsonObject root = doc.object();

    // Clear existing recipes before import
    m_database->clearAllWorkbenches();

    // Iterate over workbenches
    for (auto it = root.begin(); it != root.end(); ++it) {
        QString workbenchName = it.key();
        QJsonArray recipesArray = it.value().toArray();

        // Add workbench
        Frontier::Workbench workbench;
        workbench.name = workbenchName;
        int workbenchId = m_database->addWorkbench(workbench);

        if (workbenchId < 0) {
            qWarning() << "Failed to add workbench:" << workbenchName;
            continue;
        }

        m_workbenchesImported++;

        // Add recipes for this workbench
        for (const QJsonValue &recipeVal : recipesArray) {
            QJsonObject recipeObj = recipeVal.toObject();

            Frontier::Recipe recipe;
            recipe.workbenchId = workbenchId;
            recipe.outputItem = recipeObj["output"].toString();
            recipe.outputQty = recipeObj["output_qty"].toInt(1);
            recipe.notes = recipeObj["notes"].toString();

            int recipeId = m_database->addRecipe(recipe);

            if (recipeId < 0) {
                qWarning() << "Failed to add recipe:" << recipe.outputItem;
                continue;
            }

            // Add ingredients
            QJsonArray inputsArray = recipeObj["inputs"].toArray();
            for (const QJsonValue &inputVal : inputsArray) {
                QJsonObject inputObj = inputVal.toObject();

                Frontier::RecipeIngredient ingredient;
                ingredient.recipeId = recipeId;
                ingredient.itemName = inputObj["item"].toString();
                ingredient.quantity = inputObj["qty"].toInt(1);

                m_database->addRecipeIngredient(ingredient);
            }

            m_recipesImported++;
        }
    }

    qDebug() << "Recipe import complete:"
             << m_workbenchesImported << "workbenches,"
             << m_recipesImported << "recipes";

    return true;
}
