/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include <reaction.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main() {
    using namespace reaction;
    using namespace std;

    // A class representing a single stock
    class Stock {
    public:
        Stock(string name, double price, int shares)
            : name(move(name)),
              currentPrice(var(price).setName(this->name + "_price")),
              sharesHeld(var(shares).setName(this->name + "_shares")) {}

        // Stock name
        const string name;

        // Current price (reactive variable)
        Var<double> currentPrice;

        // Number of shares held (reactive variable)
        Var<int> sharesHeld;

        // Current position value (computed value)
        Calc<double> value = calc([](auto current, auto shares) { return current * shares; },
                                  currentPrice,
                                  sharesHeld)
                                 .setName(name + "_value");
    };

    // Create a portfolio of stocks
    vector<Stock> portfolio = {Stock("AAPL", 182.52, 100),
                               Stock("MSFT", 407.54, 50),
                               Stock("GOOGL", 153.78, 75),
                               Stock("AMZN", 177.23, 40)};

    // Total value of all positions (computed value)
    Calc<double> totalValue = calc([&] {
                                  double sum = 0.0;
                                  for (auto& stock : portfolio) {
                                      sum += stock.value();
                                  }
                                  return sum;
                              }).setName("total_value");

    // Cash balance (reactive variable)
    auto cashBalance = var(10000.0).setName("cash_balance");

    // Total net worth = holdings + cash (computed value)
    Calc<double> netWorth = calc([&] { return totalValue() + cashBalance(); }).setName("net_worth");

    // Print the current state of the portfolio
    auto printPortfolio = action([&] {
        cout << "\n=== Portfolio Update ===" << endl;
        cout << fixed << setprecision(2);

        // Print header
        cout << left << setw(8) << "Stock" << right << setw(10) << "Price" << setw(10) << "Shares"
             << setw(12) << "Value" << endl;
        cout << string(40, '-') << endl;

        // Print each stock's details
        for (auto& stock : portfolio) {
            cout << left << setw(8) << stock.name << right << setw(10) << stock.currentPrice()
                 << setw(10) << stock.sharesHeld() << setw(12) << stock.value() << endl;
        }

        // Print summary info
        cout << string(40, '-') << endl;
        cout << left << setw(18) << "Total Holdings:" << right << setw(20) << totalValue() << endl;
        cout << left << setw(18) << "Cash Balance:" << right << setw(20) << cashBalance() << endl;
        cout << left << setw(18) << "Net Worth:" << right << setw(20) << netWorth() << endl;
        cout << "=====================================\n";
    });

    // Market opens - batch update all stock prices
    cout << "\n[Market Open: Updating All Stock Prices]\n";
    {
        auto marketOpenBatch = batch([&] {
            portfolio[0].currentPrice.value(183.25); // AAPL up
            portfolio[1].currentPrice.value(405.80); // MSFT down
            portfolio[2].currentPrice.value(155.45); // GOOGL up
            portfolio[3].currentPrice.value(178.75); // AMZN up
        });
        marketOpenBatch.execute();
    }

    // Execute trades - batch buy/sell operations
    cout << "\n[Execute Trades: Buy AAPL, Sell MSFT]\n";
    {
        auto tradeBatch = batch([&] {
            // Buy 50 shares of AAPL
            double aaplCost = 183.25 * 50;
            portfolio[0].sharesHeld.value(portfolio[0].sharesHeld() + 50);
            cashBalance.value(cashBalance() - aaplCost);

            // Sell 20 shares of MSFT
            double msftProceeds = 405.80 * 20;
            portfolio[1].sharesHeld.value(portfolio[1].sharesHeld() - 20);
            cashBalance.value(cashBalance() + msftProceeds);
        });
        tradeBatch.execute();
    }

    // Market fluctuation - some stock prices change
    cout << "\n[Market Fluctuation: AAPL and GOOGL Price Changes]\n";
    {
        auto fluctuationBatch = batch([&] {
            portfolio[0].currentPrice.value(182.90); // AAPL slightly down
            portfolio[2].currentPrice.value(156.75); // GOOGL continues up
        });
        fluctuationBatch.execute();
    }

    // Dividend payout - batch update cash balance
    cout << "\n[Dividend Payout: AAPL and MSFT Issue Dividends]\n";
    {
        auto dividendBatch = batch([&] {
            double aaplDividend = 0.24 * portfolio[0].sharesHeld();
            double msftDividend = 0.75 * portfolio[1].sharesHeld();
            cashBalance.value(cashBalance() + aaplDividend + msftDividend);
        });
        dividendBatch.execute();
    }

    return 0;
}
