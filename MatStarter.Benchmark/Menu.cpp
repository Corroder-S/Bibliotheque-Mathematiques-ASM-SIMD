#include "Menu.h"
#include <iomanip>

BenchmarkMenu::BenchmarkMenu(unsigned int seed) : rng(seed)
{
    operations.push_back({"TransformPointBatch", Ref_TransformPointBatch, SSE_TransformPointBatch});
    operations.push_back({ "DotProduct", Ref_DotProduct, SSE_DotProduct });
}

void BenchmarkMenu::MainMenu()
{
    while (true)
    {
        for (int i = 0; i < (int)operations.size(); ++i)
            std::cout << i+1 << ". " << operations[i].name << "\n";
        std::cout << "0. Quitter\n> ";
        int choice = 0;
        std::cin >> choice;
        if (choice == 0) break;
        if (choice < 1 || choice > (int)operations.size()) { std::cout << "Mauvais input\n"; continue; }
        std::cout << "Taille : 1) 10  2) 1000  3) 100000\n> ";
        int sizeChoice = 0;
        std::cin >> sizeChoice;
        const std::size_t sizes[] = {10, 1000, 100000};
        if (sizeChoice < 1 || sizeChoice > 3) { std::cout << "Mauvais input\n"; continue; }
        runBenchmark(choice - 1, sizes[sizeChoice - 1]);
    }
}

void BenchmarkMenu::runBenchmark(std::size_t functionID, std::size_t count)
{
    BenchmarkData data = GenerateData(count, rng);
    
    
    const auto& op = operations[functionID];
    const auto ref = op.ref(data);
    const auto sse = op.sse(data);
    const auto err_ref = (ref.maximumMs - ref.minimumMs)/ ref.medianMs * 100; 
    const auto err_sse = (sse.maximumMs - sse.minimumMs)/ sse.medianMs * 100;
    
    std::cout << std::left
              << std::setw(15) << "Version"
              << std::setw(12) << "med ms"
              << std::setw(12) << "min ms"
              << std::setw(12) << "max ms"
              << std::setw(10) << "err%"
              << std::setw(10) << "ratio"
              << "\n";
    
    for (int i = 0; i < 70; ++i)
    {
        std::cout << "-" ;
    }
    std::cout << std::endl;
    // Ligne de données
    std::cout << std::left
              << std::setw(15) << "Ref"
              << std::setw(12) << ref.medianMs
              << std::setw(12) << ref.minimumMs
              << std::setw(12) << ref.maximumMs
              << std::setw(10) << err_ref << "%"
              << std::setw(10) << "-"
              << "\n";
    std::cout << std::left
          << std::setw(15) << "SSE"
          << std::setw(12) << sse.medianMs
          << std::setw(12) << sse.minimumMs
          << std::setw(12) << sse.maximumMs
          << std::setw(10) << err_sse << "%"
          << std::setw(10) << ref.medianMs / sse.medianMs << "%"
          << "\n";
}