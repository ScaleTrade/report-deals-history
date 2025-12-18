#pragma once

#include <vector>
#include <sstream>
#include <string>
#include <unordered_map>
#include "Structures.h"
#include <rapidjson/document.h>
#include "ast/Ast.hpp"
#include "sbxTableBuilder/SBXTableBuilder.hpp"
#include "structures/PluginStructures.h"
#include "utils/Utils.h"

using namespace ast;

extern "C" {
    void AboutReport(rapidjson::Value& request,
                     rapidjson::Value& response,
                     rapidjson::Document::AllocatorType& allocator,
                     CServerInterface* server);

    void DestroyReport();

    void CreateReport(rapidjson::Value& request,
                     rapidjson::Value& response,
                     rapidjson::Document::AllocatorType& allocator,
                     CServerInterface* server);
}