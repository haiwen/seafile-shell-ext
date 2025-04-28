﻿#include "ext-common.h"
#include "ext-utils.h"

#include <string>
#include <map>
#include <memory>

namespace utils = seafile::utils;

using std::string;

namespace {

string getWin32Locale()
{
  LCID lcid;
  wchar_t iso639[10];

  lcid = GetThreadLocale ();

  if (!GetLocaleInfo (lcid, LOCALE_SISO639LANGNAME, iso639, sizeof (iso639)))
      return "C";

  return utils::wStringToUtf8 (iso639);
}

class I18NHelper {
public:
    static I18NHelper *instance();

    string getString(const string& src) const;

private:
    I18NHelper();
    static I18NHelper *singleton_;

    void initChineseDict();
    void initGermanDict();
    void initFrenchDict();

    std::map<string, string> lang_dict_;
};


I18NHelper* I18NHelper::singleton_;

I18NHelper *I18NHelper::instance() {
    if (!singleton_) {
        singleton_ = new I18NHelper();
    }
    return singleton_;
}

void I18NHelper::initChineseDict()
{
    lang_dict_["get share link"] = "获取共享链接";
    lang_dict_["get internal link"] = "获取内部链接";
    lang_dict_["lock this file"] = "锁定该文件";
    lang_dict_["unlock this file"] = "解锁该文件";
    lang_dict_["share to a user"] = "共享给其他用户";
    lang_dict_["share to a group"] = "共享给群组";
    lang_dict_["view file history"] = "查看文件历史";
    lang_dict_["download"] = "下载";
    lang_dict_["locked by ..."] = "显示文件锁定者";
    lang_dict_["get upload link"] = "获取上传链接";
}

void I18NHelper::initGermanDict()
{
    lang_dict_["get share link"] = "Download-Link erstellen";
    lang_dict_["get internal link"] = "Internen Link erstellen";
    lang_dict_["lock this file"] = "Datei sperren";
    lang_dict_["unlock this file"] = "Datei entsperren";
    lang_dict_["share to a user"] = "Freigabe für Benutzer";
    lang_dict_["share to a group"] = "Freigabe für Gruppe";
    lang_dict_["view file history"] = "Vorgängerversionen";
    lang_dict_["download"] = "Herunterladen";
    lang_dict_["locked by ..."] = "Gesperrt durch …";
    lang_dict_["get upload link"] = "Upload-Link erstellen";
}

void I18NHelper::initFrenchDict()
{
    lang_dict_["get share link"] = "Créer lien de téléchargement";
    lang_dict_["get internal link"] = "Créer lien interne";
    lang_dict_["lock this file"] = "Verrouiller le fichier";
    lang_dict_["unlock this file"] = "Déverrouiller le fichier";
    lang_dict_["share to a user"] = "Partager avec un utilisateur";
    lang_dict_["share to a group"] = "Partager avec un groupe";
    lang_dict_["view file history"] = "Versions précédentes";
    lang_dict_["download"] = "download";
    lang_dict_["locked by ..."] = "Verrouillé par ...";
    lang_dict_["get upload link"] = "Créer lien d'envoi";
}

I18NHelper::I18NHelper()
{
    string locale = getWin32Locale();
    if (locale == "zh") {
        initChineseDict();
    } else if (locale == "de") {
        initGermanDict();
    } else if (locale == "fr") {
        initFrenchDict();
    }
}

string I18NHelper::getString(const string& src) const
{
    auto value = lang_dict_.find(src);
    return value != lang_dict_.end() ? value->second : src;
}

} // namespace

namespace seafile {

string getString(const string& src)
{
    string value = I18NHelper::instance()->getString(src);
    return value;
}

}
