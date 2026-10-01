#include "Menu.h"
#include <iomanip>

BenchmarkMenu::BenchmarkMenu(unsigned int seed) : rng(seed)
{
    operations.push_back({"TransformPointBatch", Ref_TransformPointBatch, SSE_TransformPointBatch});
    operations.push_back({ "DotProduct_AOS", Ref_DotProduct, SSE_DotProduct });
    operations.push_back({ "DotProduct_SOA", Ref_DotProduct_SOA, SSE_DotProduct_SOA });
}

void BenchmarkMenu::MainMenu()
{
    while (true)
    {
        int choice = 0;
        while (true)
        {
            for (int i = 0; i < (int)operations.size(); ++i)
                std::cout << i+2 << ". " << operations[i].name << "\n";
            std::cout << "1. Changer la seed\n";
            std::cout << "0. Quitter\n> ";
            std::cin >> choice;

            if (choice == 0) break;
            if (choice == 1)
            {
                unsigned int newSeed;
                std::cout << "Nouvelle seed : ";
                std::cin >> newSeed;
                rng.seed(newSeed);
                break;
            }
            if (choice > 1) break;
        }
        if (choice == 0) break;
        if (choice == 1) continue;

        if (choice > (int)operations.size() + 1) { std::cout << "Mauvais input\n"; continue; }
        std::cout << "Taille : 1) 10  2) 1000  3) 100000  4) Au choix\n> ";
        int sizeChoice = 0;
        std::cin >> sizeChoice;
        std::size_t sizes[] = {10, 1000, 100000, 0};
        if (sizeChoice == 4)
        {
            std::cout << "Entrez la taille : ";
            std::cin >> sizes[3];
        }
        if (sizeChoice < 1 || sizeChoice > 4) { std::cout << "Mauvais input\n"; continue; }
        runBenchmark(choice - 2, sizes[sizeChoice - 1]);
    }
}

void BenchmarkMenu::runBenchmark(std::size_t functionID, std::size_t count)
{
    const auto& op = operations[functionID];

    BenchmarkData dataRef = GenerateData(count, rng);
    const auto ref = op.ref(dataRef);

    BenchmarkData dataSSE = GenerateData(count, rng);
    const auto sse = op.sse(dataSSE);

    const auto err_ref = (ref.maximumMs - ref.minimumMs) / ref.medianMs * 100;
    const auto err_sse = (sse.maximumMs - sse.minimumMs) / sse.medianMs * 100;
    const double ratio = (sse.medianMs > 0.0) ? ref.medianMs / sse.medianMs : 0.0;

    std::cout << "\n" << op.name << " | n=" << count << "\n";
    std::cout << std::left
              << std::setw(15) << "Version"
              << std::setw(12) << "med ms"
              << std::setw(12) << "min ms"
              << std::setw(12) << "max ms"
              << std::setw(10) << "err%"
              << std::setw(10) << "ratio"
              << "\n";
    std::cout << std::string(71, '-') << "\n";

    std::cout << std::left
              << std::setw(15) << "Ref"
              << std::setw(12) << ref.medianMs
              << std::setw(12) << ref.minimumMs
              << std::setw(12) << ref.maximumMs
              << std::setw(9)  << err_ref << "%"
              << std::setw(10) << "-"
              << "\n";

    std::cout << std::left
              << std::setw(15) << "SSE"
              << std::setw(12) << sse.medianMs
              << std::setw(12) << sse.minimumMs
              << std::setw(12) << sse.maximumMs
              << std::setw(9)  << err_sse << "%"
              << std::setw(9)  << ratio << "x"
              << "\n\n";
}