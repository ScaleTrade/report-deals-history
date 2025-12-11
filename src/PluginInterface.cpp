#include "PluginInterface.hpp"

#include <iomanip>

extern "C" void AboutReport(rapidjson::Value& request,
                            rapidjson::Value& response,
                            rapidjson::Document::AllocatorType& allocator,
                            CServerInterface* server) {
    response.AddMember("version", 1, allocator);
    response.AddMember("name", Value().SetString("Trades History report", allocator), allocator);
    response.AddMember("description",
    Value().SetString("Summarized trades performed by a selected trader group over a specified period. "
                                 "Includes operation ID, date, symbol, price, profit, and account details.",
             allocator), allocator);
    response.AddMember("type", REPORT_RANGE_GROUP_TYPE, allocator);
}

extern "C" void DestroyReport() {}

extern "C" void CreateReport(rapidjson::Value& request,
                             rapidjson::Value& response,
                             rapidjson::Document::AllocatorType& allocator,
                             CServerInterface* server) {
    std::string group_mask;
    int from;
    int to;
    if (request.HasMember("group") && request["group"].IsString()) {
        group_mask = request["group"].GetString();
    }
    if (request.HasMember("from") && request["from"].IsNumber()) {
        from = request["from"].GetInt();
    }
    if (request.HasMember("to") && request["to"].IsNumber()) {
        to = request["to"].GetInt();
    }

    std::unordered_map<std::string, Total> totals_map;
    std::vector<TradeRecord> trades_vector;
    std::vector<GroupRecord> groups_vector;

    try {
        server->GetCloseTradesByGroup(group_mask, from, to, &trades_vector);
        server->GetAllGroups(&groups_vector);
    } catch (const std::exception& e) {
        std::cerr << "[TradesHistoryReportInterface]: " << e.what() << std::endl;
    }

    // Лямбда для поиска валюты аккаунта по группе
    auto get_group_currency = [&](const std::string& group_name) -> std::string {
        for (const auto& group : groups_vector) {
            if (group.group == group_name) {
                return group.currency;
            }
        }
        return "N/A"; // группа не найдена - валюта не определена
    };

    // Лямбда подготавливающая значения double для вставки в AST (округление до 2-х знаков)
    auto format_double_for_AST = [](double value) -> std::string {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << value;
        return oss.str();
    };

    TableBuilder table_builder("TradesHistoryReportTable");

    table_builder.SetIdColumn("order");
    table_builder.SetOrderBy("order", "DESC");
    table_builder.EnableRefreshButton(false);
    table_builder.EnableBookmarksButton(false);
    table_builder.EnableExportButton(true);
    table_builder.EnableTotal(true);
    table_builder.SetTotalDataTitle("TOTAL");

    table_builder.AddColumn({"order", "ORDER"});
    table_builder.AddColumn({"login", "LOGIN"});
    table_builder.AddColumn({"name", "NAME"});
    table_builder.AddColumn({"close_time", "CLOSE_TIME"});
    table_builder.AddColumn({"type", "TYPE"});
    table_builder.AddColumn({"symbol", "SYMBOL"});
    table_builder.AddColumn({"volume", "VOLUME"});
    table_builder.AddColumn({"close_price", "CLOSE_PRICE"});
    table_builder.AddColumn({"sl", "S / L"});
    table_builder.AddColumn({"tp", "T / P"});
    table_builder.AddColumn({"commission", "COMMISSION"});
    table_builder.AddColumn({"storage", "SWAP"});
    table_builder.AddColumn({"profit", "AMOUNT"});
    table_builder.AddColumn({"currency", "CURRENCY"});
    table_builder.AddColumn({"comment", "COMMENT"});

    for (const auto& trade : trades_vector) {
        AccountRecord account;
        
        try {
            server->GetAccountByLogin(trade.login, &account);
        } catch (const std::exception& e) {
            std::cerr << "[TradesHistoryReportInterface]: " << e.what() << std::endl;
        }
    
        const std::string currency = get_group_currency(account.group);
        double multiplier = 1;

        totals_map["USD"].volume += trade.volume;

        if (currency == "USD") {
            totals_map["USD"].commission += trade.commission;
            totals_map["USD"].profit += trade.profit;
        } else {
            try {
                server->CalculateConvertRateByCurrency(currency, "USD", trade.cmd, &multiplier);
            } catch (const std::exception& e) {
                std::cerr << "[TradesHistoryReportInterface]: " << e.what() << std::endl;
            }

            totals_map["USD"].commission += trade.commission * multiplier;
            totals_map["USD"].profit += trade.profit * multiplier;
        }

        table_builder.AddRow({
            {"order", std::to_string(trade.order)},
            {"login", std::to_string(trade.login)},
            {"name", account.name},
            {"close_time", utils::FormatTimestampToString(trade.close_time)},
            {"type", trade.cmd == 0 ? "buy" : "sell"},
            {"symbol", trade.symbol},
            {"volume", std::to_string(trade.volume)},
            {"close_price", std::to_string(trade.close_price * multiplier)},
            {"sl", std::to_string(trade.sl)},
            {"tp", std::to_string(trade.tp)},
            {"commission", std::to_string(trade.commission * multiplier)},
            {"storage", std::to_string(trade.storage * multiplier)},
            {"profit", format_double_for_AST(trade.profit * multiplier)},
            {"currency", "USD"},
            {"comment", trade.comment}
        });
    }

    // Total row
    JSONArray totals_array;
    totals_array.emplace_back(JSONObject{
        {"volume", totals_map["USD"].volume},
        {"commission", totals_map["USD"].commission},
        {"profit", totals_map["USD"].profit},
        {"currency", "USD"}
    });

    table_builder.SetTotalData(totals_array);

    const JSONObject table_props = table_builder.CreateTableProps();
    const Node table_node = Table({}, table_props);

    // Total report
    const Node report = div({
        h1({text("Trades History Report")}),
        table_node
    });

    utils::CreateUI(report, response, allocator);
}
