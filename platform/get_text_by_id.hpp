#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace platform
{
// class GetTextById is ready to work with different sources of text strings.
// For the time being it's only strings for TTS.
enum class TextSource
{
  TtsSound = 0, //!< Maneuvers text to speech strings.
  Countries,    //!< Countries names strings.
  Cuisines      //!< OSM cuisines translations.
};

class GetTextById;
using TGetTextByIdPtr = std::unique_ptr<GetTextById>;
using TTranslations = std::vector<std::pair<std::string, std::string>>;

/// GetTextById represents text messages which are saved in textsDir
/// in a specified locale.
class GetTextById
{
public:
  /// @return a pair of a text string in a specified locale for textId and a boolean flag.
  /// If textId is found in m_localeTexts then the boolean flag is set to true.
  /// The boolean flag is set to false otherwise.
  std::string operator()(std::string const & textId) const;
  std::string GetLocale() const { return m_locale; }
  TTranslations GetAllSortedTranslations() const;

private:
  friend TGetTextByIdPtr GetTextByIdFactory(TextSource textSource, std::string const & localeName);
  friend TGetTextByIdPtr ForTestingGetTextByIdFactory(std::string const & jsonBuffer,
                                                      std::string const & localeName);
  friend TGetTextByIdPtr MakeGetTextById(std::string const & jsonBuffer, std::string const & localeName);

  GetTextById(std::string const & jsonBuffer, std::string const & localeName);
  void InitFromJson(std::string const & jsonBuffer);
  /// \note IsValid is used only in factories and shall be private.
  bool IsValid() const { return !m_localeTexts.empty(); }

  std::string m_locale;
  std::unordered_map<std::string, std::string> m_localeTexts;
};

/// Factories to create GetTextById instances.
/// If TGetTextByIdPtr is created by GetTextByIdFactory or ForTestingGetTextByIdFactory
/// there are only two possibities:
/// * a factory returns a valid instance
/// * a factory returns nullptr
TGetTextByIdPtr GetTextByIdFactory(TextSource textSource, std::string const & localeName);
TGetTextByIdPtr ForTestingGetTextByIdFactory(std::string const & jsonBuffer, std::string const & localeName);

/// \bried fills jsonBuffer with json file in twine format with strings in a language of localeName.
/// @return true if no error was happened and false otherwise.
bool GetJsonBuffer(platform::TextSource textSource, std::string const & localeName, std::string & jsonBuffer);
}  // namespace platform
