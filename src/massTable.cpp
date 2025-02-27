#include "nuclear-data-reader/massTable.hpp"

#include "nuclear-data-reader/converter.hpp"
#include "nuclear-data-reader/isotope.hpp"
#include "nuclear-data-reader/nubase_data.hpp"
#include <type_traits>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <algorithm>
#include <array>
#include <climits>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>
#include <utility>


void MassTable::setFilePaths() const
{
  switch (year)
    {
      default:
      case 2003:
        NUBASE_masstable = MassTable::getAbsolutePath() / "2003" / "nubtab03.asc";
        AME_masstable    = MassTable::getAbsolutePath() / "2003" / "mass.mas03";
        AME_reaction_1   = MassTable::getAbsolutePath() / "2003" / "rct1.mas03";
        AME_reaction_2   = MassTable::getAbsolutePath() / "2003" / "rct2.mas03";
        break;
      case 2012:
        NUBASE_masstable = MassTable::getAbsolutePath() / "2012" / "nubtab12.asc";
        AME_masstable    = MassTable::getAbsolutePath() / "2012" / "mass.mas12";
        AME_reaction_1   = MassTable::getAbsolutePath() / "2012" / "rct1.mas12";
        AME_reaction_2   = MassTable::getAbsolutePath() / "2012" / "rct2.mas12";
        break;
      case 2016:
        NUBASE_masstable = MassTable::getAbsolutePath() / "2016" / "nubase2016.txt";
        AME_masstable    = MassTable::getAbsolutePath() / "2016" / "mass16.txt";
        AME_reaction_1   = MassTable::getAbsolutePath() / "2016" / "rct1-16.txt";
        AME_reaction_2   = MassTable::getAbsolutePath() / "2016" / "rct2-16.txt";
        break;
      case 2020:
        NUBASE_masstable = MassTable::getAbsolutePath() / "2020" / "nubase_1.mas20";
        AME_masstable    = MassTable::getAbsolutePath() / "2020" / "mass.mas20";
        AME_reaction_1   = MassTable::getAbsolutePath() / "2020" / "rct1.mas20";
        AME_reaction_2   = MassTable::getAbsolutePath() / "2020" / "rct2.mas20";
        break;
    }
}


bool MassTable::populateInternalMassTable()
{
  setFilePaths();

  // Read mass table
  if (!readNUBASE(NUBASE_masstable))
    {
      fmt::print("Nuclear data has not been read, exiting...");
      return false;
    }

  if (!readAMEMassFile(AME_masstable))
    {
      fmt::print("Values from AME were not read.\n");
    }

  if (!readAMEReactionFileOne(AME_reaction_1))
    {
      fmt::print("Reaction values from first file not read.\n");
    }

  if (!readAMEReactionFileTwo(AME_reaction_2))
    {
      fmt::print("Reaction values from first file not read.\n");
    }

  return mergeData();
}


AME::Data MassTable::parseAMEMassFormat(const std::string& line) const
{
  AME::Data data(line, year);

  data.setA();
  data.setZ();
  data.setN();

  data.setMassExcess();
  data.setMassExcessError();

  data.setBindingEnergyPerA();
  data.setBindingEnergyPerAError();

  data.setBetaDecayEnergy();
  data.setBetaDecayEnergyError();

  data.setAtomicMass();
  data.setAtomicMassError();

  return data;
}


std::vector<AME::Data>::iterator MassTable::getMatchingIsotope(const std::string& line, const int reactionFile) const
{
  // Check that the mass table has already been populated
  if (ameDataTable.empty())
    {
      fmt::print("Mass table must be read first\n");
      return ameDataTable.end();
    }

  const AME::Data data(line, year);

  // A & Z are in the same place for both reaction files, but lets not assume they will be forever
  const int A = (reactionFile == 1) ? data.getReaction_1_A(line) : data.getReaction_2_A(line);
  const int Z = (reactionFile == 1) ? data.getReaction_1_Z(line) : data.getReaction_2_Z(line);

  // Look for the correct isotope in the existing data table
  auto isotope = std::find_if(
      ameDataTable.begin(), ameDataTable.end(), [A, Z](const auto& ame) -> bool { return (ame.A == A && ame.Z == Z); });

  // Get out if it doesn't exist
  if (isotope == ameDataTable.end())
    {
      fmt::print("**WARNING**: No matching mass data found for A={}, Z={}\n", A, Z);
      return ameDataTable.end();
    }

  // As we don't construct a AME::Data instance we need to make sure we are reading the correct line/data
  isotope->full_data = line;
  // Make sure the line is the correct length so accessing line locations doesn't cause a crash
  isotope->full_data.resize(line_length, ' ');
  // Strip '#' characters (May not be needed)
  isotope->setExperimental();

  return isotope;
}


bool MassTable::parseAMEReactionTwoFormat(const std::string& line) const
{
  auto isotope = getMatchingIsotope(line, 2);

  if (isotope == ameDataTable.end())
    {
      return false;
    }

  isotope->setOneNeutronSeparationEnergy();
  isotope->setOneNeutronSeparationEnergyError();

  isotope->setOneProtonSeparationEnergy();
  isotope->setOneProtonSeparationEnergyError();

  isotope->setQQuadrupleBetaMinusEnergy();
  isotope->setQQuadrupleBetaMinusEnergyError();

  isotope->setQDAlphaEnergy();
  isotope->setQDAlphaEnergyError();

  isotope->setQPAlphaEnergy();
  isotope->setQPAlphaEnergyError();

  isotope->setQNAlphaEnergy();
  isotope->setQNAlphaEnergyError();

  return true;
}


bool MassTable::parseAMEReactionOneFormat(const std::string& line) const
{
  auto isotope = getMatchingIsotope(line, 1);

  if (isotope == ameDataTable.end())
    {
      return false;
    }

  isotope->setTwoNeutronSeparationEnergy();
  isotope->setTwoNeutronSeparationEnergyError();

  isotope->setTwoProtonSeparationEnergy();
  isotope->setTwoProtonSeparationEnergyError();

  isotope->setQAlphaEnergy();
  isotope->setQAlphaEnergyError();

  isotope->setQDoubleBetaMinusEnergy();
  isotope->setQDoubleBetaMinusEnergyError();

  isotope->setQEpsilonPEnergy();
  isotope->setQEpsilonPEnergyError();

  isotope->setQBetaMinusNEnergy();
  isotope->setQBetaMinusNEnergyError();

  return true;
}


bool MassTable::readAMEMassFile(const std::filesystem::path& ameTable) const
{
  fmt::print("Reading {} for AME mass excess values <--", ameTable);

  if (!std::filesystem::exists(ameTable))
    {
      fmt::print("\n***ERROR***: {} does not exist?\n\n", ameTable);
      return false;
    }

  std::ifstream file(ameTable, std::ios::binary);

  const AME::Data data("", year);
  int l = 0;
  for (l = 0; l < data.mass_position.HEADER; ++l)
    {
      file.ignore((std::numeric_limits<std::streamsize>::max) (), '\n');
    }

  std::string line;
  while (std::getline(file, line) && l < data.mass_position.FOOTER)
    {
      ameDataTable.emplace_back(parseAMEMassFormat(line));
      ++l;
    }

  file.close();

  fmt::print("--> done\n");
  return true;
}


bool MassTable::readAMEReactionFileOne(const std::filesystem::path& reactionFile) const
{
  fmt::print("Reading {} for reaction data <--", reactionFile);

  if (!std::filesystem::exists(reactionFile))
    {
      fmt::print("\n***ERROR***: {} does not exist?\n\n", reactionFile);
      return false;
    }

  std::ifstream file(reactionFile, std::ios::binary);

  const AME::Data data("", year);
  int l = 0;
  for (l = 0; l < data.r1_position.R1_HEADER; ++l)
    {
      file.ignore((std::numeric_limits<std::streamsize>::max) (), '\n');
    }

  std::string line;
  while (std::getline(file, line) && l < data.r1_position.R1_FOOTER)
    {
      ++l;
      if (!parseAMEReactionOneFormat(line))
        {
          fmt::print("**WARNING**: No matching isotope found for\n{}\n", line);
        }
    }

  fmt::print("--> done\n");
  return true;
}


bool MassTable::readAMEReactionFileTwo(const std::filesystem::path& reactionFile) const
{
  fmt::print("Reading {} for reaction data <--", reactionFile);

  if (!std::filesystem::exists(reactionFile))
    {
      fmt::print("\n***ERROR***: {} does not exist?\n\n", reactionFile);
      return false;
    }

  std::ifstream file(reactionFile, std::ios::binary);

  const AME::Data data("", year);
  int l = 0;
  for (l = 0; l < data.r2_position.R2_HEADER; ++l)
    {
      file.ignore((std::numeric_limits<std::streamsize>::max) (), '\n');
    }

  std::string line;
  while (std::getline(file, line) && l < data.r2_position.R2_FOOTER)
    {
      ++l;
      // skip repeated header which only happens in the 2020 file
      if (line.find("1 A  elt") != std::string::npos)
        {
          continue;
        }

      if (!parseAMEReactionTwoFormat(line))
        {
          fmt::print("**WARNING**: No matching isotope found for\n{}\n", line);
        }
    }

  fmt::print("--> done\n");
  return true;
}


NUBASE::Data MassTable::parseNUBASEFormat(const std::string& line) const
{
  NUBASE::Data data(line, year);

  data.setExperimental();

  data.setA();
  data.setZ();
  data.setSymbol(Converter::ZToSymbol(data.Z));
  data.setN();

  data.setState();

  // For the non ground-state entires we have enough data to attach this level to the appropriate isotope
  if (data.level > 0)
    {
      return data;
    }

  data.setMassExcess();
  data.setMassExcessError();

  data.setSpinParity();

  data.setHalfLife();

  // Discovery year was added after 2003
  if (year != 2003)
    {
      data.setYear();
    }

  data.setDecayMode(year);

  if (data.decay == "stable")
    {
      pnSide.at(data.Z) = true;
    }

  data.setNeutronOrProtonRich(pnSide.at(data.Z));

  return data;
}


bool MassTable::readNUBASE(const std::filesystem::path& nubaseTable)
{
  fmt::print("Reading {} for nuclear values <--", nubaseTable);
  std::ifstream file(nubaseTable, std::ios::binary);

  if (!file.is_open())
    {
      fmt::print("\n***ERROR***: {} couldn't be opened, does it exist?\n\n", nubaseTable);
      return false;
    }

  const NUBASE::Data data("", year);
  for (int i = 0; i < data.position.HEADER; ++i)
    {
      file.ignore((std::numeric_limits<std::streamsize>::max) (), '\n');
    }

  std::string line;

  while (std::getline(file, line))
    {
      if (line.find("non-exist") != std::string::npos)
        {
          continue;
        }

      auto nuclide = parseNUBASEFormat(line);

      // Merge isomers with their existing ground-state
      // We don't want to add a new entry to the vector so skip that step
      if (nuclide.level != 0)
        {
          nuclide.setIsomerData(nubaseDataTable);
          continue;
        }

      nubaseDataTable.emplace_back(nuclide);
    }

  file.close();

  fmt::print("--> done\n");
  return true;
}


bool MassTable::mergeData(const int verbosity) const
{
  if (ameDataTable.size() != nubaseDataTable.size())
    {
      fmt::print("**WARNING** The AME data ({}) has a different number of isotopes to NUBASE ({})\n",
                 ameDataTable.size(),
                 nubaseDataTable.size());
    }

  for (const auto& nubase : nubaseDataTable)
    {
      const auto ame = std::find_if(ameDataTable.cbegin(), ameDataTable.cend(), [&nubase](const auto n) -> bool {
        return (n.A == nubase.A && n.Z == nubase.Z);
      });

      if (ame != ameDataTable.end())
        {
          // Isotope(AME, NUBASE))
          fullDataTable.emplace_back(Isotope(*ame, nubase));
        }
      else if (verbosity > 0)
        {
          fmt::print("{} {}\n", nubase.A, nubase.Z);
        }
    }

  return true;
}


// bool MassTable::outputTableToCSV() const
//{
//  auto outfile = NUBASE_masstable;
//  outfile.replace_extension(".csv");
//
//  fmt::print("New file: {}\n", outfile);
//  std::ofstream out(outfile);
//
//  if (!out.is_open())
//    {
//      fmt::print("\n***ERROR***: {} couldn't be opened", outfile);
//      return false;
//    }
//
//  // fmt::print(out, "{}\n", Isotope::CSVHeader());
//  // for (const auto& isotope : theTable)
//  //  {
//  //    fmt::print(out, "{}\n", isotope.writeAsCSV());
//  //  }
//  out.close();
//
//  return true;
//}


bool MassTable::outputTableToJSON() const
{
  const std::filesystem::path outfile = fmt::format("masstable_{}.json", year);

  fmt::print("New file: {}\n", outfile);
  std::ofstream out(outfile);

  if (!out.is_open())
    {
      fmt::print("\n***ERROR***: {} couldn't be opened", outfile);
      return false;
    }

  fmt::print(out, "[\n");
  // The final element can't have a trailing comma, otherwise we'd use a range loop here
  for (auto isotope = fullDataTable.cbegin(); isotope != fullDataTable.cend(); ++isotope)
    {
      fmt::print(out, "{}{}", isotope->writeAsJSON(), (isotope != std::prev(fullDataTable.end(), 1)) ? ",\n" : "");
    }
  fmt::print(out, "\n]\n");
  out.close();

  return true;
}
