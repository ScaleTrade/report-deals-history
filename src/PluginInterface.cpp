#include "PluginInterface.h"

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
    std::unordered_map<int, AccountRecord> accounts;

    try {
        server->GetCloseTradesByGroup(group_mask, from, to, &trades_vector);
        server->GetAllGroups(&groups_vector);
    } catch (const std::exception& e) {
        std::cerr << "[TradesHistoryReportInterface]: " << e.what() << std::endl;
    }

    // Main table
    TableBuilder table_builder("TradesHistoryReportTable");

    // Table props
    table_builder.SetIdColumn("order");
    table_builder.SetOrderBy("order", "DESC");
    table_builder.EnableAutoSave(false);
    table_builder.EnableRefreshButton(false);
    table_builder.EnableBookmarksButton(false);
    table_builder.EnableExportButton(true);
    table_builder.EnableTotal(true);
    table_builder.SetTotalDataTitle("TOTAL");

    // Filters
    FilterConfig search_filter;
    search_filter.type = FilterType::Search;

    FilterConfig date_time_filter;
    date_time_filter.type = FilterType::DateTime;

    // Columns
    table_builder.AddColumn({"order", "ORDER" , 1, search_filter});
    table_builder.AddColumn({"login", "LOGIN", 2, search_filter});
    table_builder.AddColumn({"name", "NAME", 3, search_filter});
    table_builder.AddColumn({"open_time", "OPEN_TIME", 4, date_time_filter});
    table_builder.AddColumn({"close_time", "CLOSE_TIME", 5, date_time_filter});
    table_builder.AddColumn({"type", "TYPE",  6, search_filter});
    table_builder.AddColumn({"symbol", "SYMBOL", 7, search_filter});
    table_builder.AddColumn({"volume", "VOLUME", 8, search_filter});
    table_builder.AddColumn({"open_price", "OPEN_PRICE", 9, search_filter});
    table_builder.AddColumn({"close_price", "CLOSE_PRICE", 10, search_filter});
    table_builder.AddColumn({"sl", "S / L", 11, search_filter});
    table_builder.AddColumn({"tp", "T / P", 12, search_filter});
    table_builder.AddColumn({"commission", "COMMISSION", 13, search_filter});
    table_builder.AddColumn({"storage", "SWAP", 14, search_filter});
    table_builder.AddColumn({"profit", "AMOUNT", 15, search_filter});
    table_builder.AddColumn({"currency", "CURRENCY", 16, search_filter});
    table_builder.AddColumn({"comment", "COMMENT", 17, search_filter});

    for (const auto& trade : trades_vector) {
        AccountRecord account;
        auto iterator = accounts.find(trade.login);

        if (iterator != accounts.end()) {
            account = iterator->second;
        } else {
            try {
                server->GetAccountByLogin(trade.login, &account);
                accounts.insert({trade.login, account});
            } catch (const std::exception& e) {
                std::cerr << "[TradesHistoryReportInterface]: " << e.what() << std::endl;
            }
        }
    
        const std::string currency = utils::GetGroupCurrencyByName(groups_vector, account.group);
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
            utils::TruncateDouble(trade.order, 0),
            utils::TruncateDouble(trade.login, 0),
            account.name,
            utils::FormatTimestampToString(trade.open_time),
            utils::FormatTimestampToString(trade.close_time),
            trade.cmd == 0 ? "buy" : "sell",
            trade.symbol,
            utils::TruncateDouble(trade.volume / 100, 0),
            utils::TruncateDouble(trade.open_price * multiplier, 2),
            utils::TruncateDouble(trade.close_price * multiplier, 2),
            utils::TruncateDouble(trade.sl, 2),
            utils::TruncateDouble(trade.tp, 2),
            utils::TruncateDouble(trade.commission * multiplier, 2),
            utils::TruncateDouble(trade.storage * multiplier, 2),
            utils::TruncateDouble(trade.profit * multiplier, 2),
            "USD",
            trade.comment
        });
    }

    // Total row
    JSONArray totals_array;
    totals_array.emplace_back(JSONObject{
        {"volume", utils::TruncateDouble(totals_map["USD"].volume, 0)},
        {"commission", utils::TruncateDouble(totals_map["USD"].commission, 2)},
        {"profit", utils::TruncateDouble(totals_map["USD"].profit, 2)},
        {"currency", "USD"}
    });

    table_builder.SetTotalData(totals_array);

    const JSONObject table_props = table_builder.CreateTableProps();
    const Node table_node = Table({}, table_props);

    // Total report
    const Node report = Column({
        h1({text("Trades History Report")}),
        table_node
    });

    utils::CreateUI(report, response, allocator);
}
