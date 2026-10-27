#pragma once
#include "V11LicensePublic.h"
#include <juce_cryptography/juce_cryptography.h>
class V11License {
public:
  bool authorized = false;
  juce::String key, owner;
  static bool validate(const juce::String &document,
                       juce::String *serial = nullptr,
                       juce::String *holder = nullptr) {
    auto v = juce::JSON::parse(document);
    if (!v.isObject() || int(v["format"]) != 1 ||
        v["scope"].toString() != "ALL" || v["term"].toString() != "PERPETUAL")
      return false;
    auto k = v["key"].toString(), o = v["owner"].toString(),
         signature = v["signature"].toString();
    if (!k.startsWith("M7G-GOLDEN-") || k.length() != 31 || o.isEmpty() ||
        o.containsAnyOf("|\r\n") || signature.length() != 512 ||
        !signature.containsOnly("0123456789abcdefABCDEF"))
      return false;
    auto payload = "M7G-LICENSE-1|" + k + "|" + o + "|ALL|PERPETUAL";
    juce::SHA256 hash(payload.toRawUTF8(), size_t(payload.getNumBytesAsUTF8()));
    juce::BigInteger decoded;
    decoded.parseString(signature, 16);
    if (!juce::RSAKey(m7gLicensePublicKey).applyToValue(decoded))
      return false;
    // Full PKCS#1 v1.5 SHA-256 encoding, including fixed padding length.
    juce::String expected = "1" + juce::String::repeatedString("ff", 202) +
                            "003031300d060960864801650304020105000420" +
                            hash.toHexString();
    if (decoded.toString(16).toLowerCase() != expected)
      return false;
    if (serial)
      *serial = k;
    if (holder)
      *holder = o;
    return true;
  }
  static juce::File userFile() {
    return juce::File::getSpecialLocation(
               juce::File::userApplicationDataDirectory)
        .getChildFile("M7G/M7G.license");
  }
  void reload() {
    authorized = false;
    key.clear();
    owner.clear();
    auto common = juce::File::getSpecialLocation(
                      juce::File::commonApplicationDataDirectory)
                      .getChildFile("M7G/M7G.license");
    for (const auto &file : {userFile(), common})
      if (file.existsAsFile() && file.getSize() <= 8192 &&
          validate(file.loadFileAsString(), &key, &owner)) {
        authorized = true;
        return;
      }
  }
  bool importFile(const juce::File &source) {
    if (source.getSize() > 8192 || !validate(source.loadFileAsString()))
      return false;
    auto destination = userFile();
    if (destination.getParentDirectory().createDirectory().failed() ||
        !destination.replaceWithText(source.loadFileAsString()))
      return false;
    reload();
    return authorized;
  }
};
