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

// v.1
// extern "C" void CreateReport(rapidjson::Value& request,
//                              rapidjson::Value& response,
//                              rapidjson::Document::AllocatorType& allocator,
//                              CServerInterface* server) {
//     // Структура накопления итогов
//     struct Total {
//         double volume;
//         double commission;
//         double profit;
//         std::string currency;
//     };
//
//     std::unordered_map<std::string, Total> totals_map;
//
//     std::string group_mask;
//     int from;
//     int to;
//     if (request.HasMember("group") && request["group"].IsString()) {
//         group_mask = request["group"].GetString();
//     }
//     if (request.HasMember("from") && request["from"].IsNumber()) {
//         from = request["from"].GetInt();
//     }
//     if (request.HasMember("to") && request["to"].IsNumber()) {
//         to = request["to"].GetInt();
//     }
//
//     std::vector<TradeRecord> trades_vector;
//     std::vector<GroupRecord> groups_vector;
//
//     try {
//         server->GetCloseTradesByGroup(group_mask, from, to, &trades_vector);
//         server->GetAllGroups(&groups_vector);
//     } catch (const std::exception& e) {
//         std::cerr << "[TradesHistoryReportInterface]: " << e.what() << std::endl;
//     }
//
//     // Лямбда для поиска валюты аккаунта по группе
//     auto get_group_currency = [&](const std::string& group_name) -> std::string {
//         for (const auto& group : groups_vector) {
//             if (group.group == group_name) {
//                 return group.currency;
//             }
//         }
//         return "N/A"; // группа не найдена - валюта не определена
//     };
//
//     // Лямбда подготавливающая значения double для вставки в AST (округление до 2-х знаков)
//     auto format_for_AST = [](double value) -> std::string {
//         std::ostringstream oss;
//         oss << std::fixed << std::setprecision(2) << value;
//         return oss.str();
//     };
//
//     auto create_table = [&](const std::vector<TradeRecord>& trades) -> Node {
//         std::vector<Node> thead_rows;
//         std::vector<Node> tbody_rows;
//         std::vector<Node> tfoot_rows;
//
//         // Thead
//         thead_rows.push_back(tr({
//             th({div({text("Order")})}),
//             th({div({text("Login")})}),
//             th({div({text("Name")})}),
//             th({div({text("Time")})}),
//             th({div({text("Type")})}),
//             th({div({text("Symbol")})}),
//             th({div({text("Volume")})}),
//             th({div({text("Price")})}),
//             th({div({text("S / L")})}),
//             th({div({text("T / P")})}),
//             th({div({text("Commission")})}),
//             th({div({text("Swap")})}),
//             th({div({text("Profit")})}),
//             th({div({text("Currency")})}),
//             th({div({text("Comment")})}),
//         }));
//
//         // Tbody
//         for (const auto& trade : trades_vector) {
//             AccountRecord account;
//
//             server->GetAccountByLogin(trade.login, &account);
//
//             std::string currency = get_group_currency(account.group);
//
//             auto& total = totals_map[currency];
//             total.currency = currency;
//             total.volume += trade.volume;
//             total.commission += trade.commission;
//             total.profit += trade.profit;
//
//             tbody_rows.push_back(tr({
//                 td({div({text(std::to_string(trade.order))})}),
//                 td({div({text(std::to_string(trade.login))})}),
//                 td({div({text(account.name)})}),
//                 td({div({text(utils::FormatTimestampToString(trade.close_time))})}),
//                 td({div({text(trade.cmd == 0 ? "buy" : "sell")})}),
//                 td({div({text(trade.symbol)})}),
//                 td({div({text(std::to_string(trade.volume))})}),
//                 td({div({text(std::to_string(trade.close_price))})}),
//                 td({div({text(std::to_string(trade.sl))})}),
//                 td({div({text(std::to_string(trade.tp))})}),
//                 td({div({text(std::to_string(trade.commission))})}),
//                 td({div({text(std::to_string(trade.storage))})}),
//                 td({div({text(format_for_AST(trade.profit))})}),
//                 td({div({text(currency)})}),
//                 td({div({text(trade.comment)})}),
//             }));
//         }
//
//         // Tfoot
//         tfoot_rows.push_back(tr({
//             td({div({text("TOTAL:")})}),
//             td({div({text("")})}),
//             td({div({text("")})}),
//             td({div({text("")})}),
//             td({div({text("")})}),
//             td({div({text("")})}),
//             td({div({text("")})}),
//             td({div({text("")})}),
//             td({div({text("")})}),
//             td({div({text("")})}),
//             td({div({text("")})}),
//             td({div({text("")})}),
//             td({div({text("")})}),
//             td({div({text("")})}),
//             td({div({text("")})})
//         }));
//
//         for (const auto& pair : totals_map) {
//             const Total& total = pair.second;
//
//             tfoot_rows.push_back(tr({
//                 td({div({text("")})}),
//                 td({div({text("")})}),
//                 td({div({text("")})}),
//                 td({div({text("")})}),
//                 td({div({text("")})}),
//                 td({div({text("")})}),
//                 td({div({text(std::to_string(total.volume))})}),
//                 td({div({text("")})}),
//                 td({div({text("")})}),
//                 td({div({text("")})}),
//                 td({div({text(std::to_string(total.commission))})}),
//                 td({div({text("")})}),
//                 td({div({text(std::to_string(total.profit))})}),
//                 td({div({text(total.currency)})}),
//                 td({div({text("")})}),
//             }));
//         }
//
//         return table({
//             thead(thead_rows),
//             tbody(tbody_rows),
//             tfoot(tfoot_rows),
//         }, props({{"className", "table"}}));
//     };
//
//     // Total report
//     const Node report = div({
//         h1({text("Trades History Report")}),
//         create_table(trades_vector),
//     });
//
//     utils::CreateUI(report, response, allocator);
// }

// v.2
extern "C" void CreateReport(rapidjson::Value& request,
                             rapidjson::Value& response,
                             rapidjson::Document::AllocatorType& allocator,
                             CServerInterface* server) {
    // Структура накопления итогов
    struct Total {
        double volume;
        double commission;
        double profit;
        std::string currency;
    };

    std::unordered_map<std::string, Total> totals_map;

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

    std::vector<TradeRecord> trades_vector;
    std::vector<GroupRecord> groups_vector;

    try {
        server->GetCloseTradesByGroup(group_mask, from, to, &trades_vector);
        server->GetAllGroups(&groups_vector);
    } catch (const std::exception& e) {
        std::cerr << "[TradesHistoryReportInterface]: " << e.what() << std::endl;
    }

    JSONObject id_column_props = {
        {"name", JSONValue("ID")},
        {"filter", JSONObject{{"type", JSONValue("search")}}}
    };

    JSONObject table_props = props({
        {"name", "MarginCall"},
        {"idCol", "id"},
        {"data", JSONArray{}},
        {"orderBy", JSONArray{JSONValue("id"), JSONValue("DESC")}},
        {"structure", JSONObject{{"id", JSONValue(id_column_props)}}}
    });

    Node table = Table({}, table_props);

    // Total report
    const Node report = div({
        h1({text("Trades History Report")}),
        table
    });

    utils::CreateUI(report, response, allocator);
}