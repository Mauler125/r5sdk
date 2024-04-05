/*H********************************************************************************/
/*!
    \File dirtylang.h

    \Description
        Country and Language Code Information

    \Notes
        This module provides country and language codes for both client and
        server code in DirtySDK.
        2-letter language codes are currently available here:
        http://en.wikipedia.org/wiki/ISO_639
        2-letter country codes are currently available here:
        http://en.wikipedia.org/wiki/ISO_3166

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 11/30/2004 (jfrank) First Version
*/
/********************************************************************************H*/

#ifndef _dirtylang_h
#define _dirtylang_h

/*!
\Moduledef DirtyLang DirtyLang
\Modulemember DirtySock
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

//! Commonly used locality values
#define LOBBYAPI_LOCALITY_UNKNOWN                       ('zzZZ')
#define LOBBYAPI_LOCALITY_EN_US                         ('enUS')
#define LOBBYAPI_LOCALITY_BLANK                         ('\0\0\0\0')
#define LOBBYAPI_LOCALITY_WILDCARD                      ('****')

#define LOBBYAPI_LOCALITY_DEFAULT                       LOBBYAPI_LOCALITY_EN_US
#define LOBBYAPI_LOCALITY_DEFAULT_STR                   "enUS"
#define LOBBYAPI_LOCALITY_UNKNOWN_STR                   "zzZZ"

//! Non-specific, commonly used country and language codes
#define LOBBYAPI_LANGUAGE_UNKNOWN                       ('zz')
#define LOBBYAPI_COUNTRY_UNKNOWN                        ('ZZ')
#define LOBBYAPI_LANGUAGE_WILDCARD                      ('**')
#define LOBBYAPI_COUNTRY_WILDCARD                       ('**')
#define LOBBYAPI_LANGUAGE_UNKNOWN_STR                   ("zz")
#define LOBBYAPI_COUNTRY_UNKNOWN_STR                    ("ZZ")

//! Languages
#define LOBBYAPI_LANGUAGE_AFAN_OROMO                    ('om')
#define LOBBYAPI_LANGUAGE_ABKHAZIAN                     ('ab')
#define LOBBYAPI_LANGUAGE_AFAR                          ('aa')
#define LOBBYAPI_LANGUAGE_AFRIKAANS                     ('af')
#define LOBBYAPI_LANGUAGE_ALBANIAN                      ('sq')
#define LOBBYAPI_LANGUAGE_AMHARIC                       ('am')
#define LOBBYAPI_LANGUAGE_ARABIC                        ('ar')
#define LOBBYAPI_LANGUAGE_ARMENIAN                      ('hy')
#define LOBBYAPI_LANGUAGE_ASSAMESE                      ('as')
#define LOBBYAPI_LANGUAGE_AYMARA                        ('ay')
#define LOBBYAPI_LANGUAGE_AZERBAIJANI                   ('az')
#define LOBBYAPI_LANGUAGE_BASHKIR                       ('ba')
#define LOBBYAPI_LANGUAGE_BASQUE                        ('eu')
#define LOBBYAPI_LANGUAGE_BENGALI                       ('bn')
#define LOBBYAPI_LANGUAGE_BHUTANI                       ('dz')
#define LOBBYAPI_LANGUAGE_BIHARI                        ('bh')
#define LOBBYAPI_LANGUAGE_BISLAMA                       ('bi')
#define LOBBYAPI_LANGUAGE_BRETON                        ('br')
#define LOBBYAPI_LANGUAGE_BULGARIAN                     ('bg')
#define LOBBYAPI_LANGUAGE_BURMESE                       ('my')
#define LOBBYAPI_LANGUAGE_BYELORUSSIAN                  ('be')
#define LOBBYAPI_LANGUAGE_CAMBODIAN                     ('km')
#define LOBBYAPI_LANGUAGE_CATALAN                       ('ca')
#define LOBBYAPI_LANGUAGE_CHINESE                       ('zh')
#define LOBBYAPI_LANGUAGE_CORSICAN                      ('co')
#define LOBBYAPI_LANGUAGE_CROATIAN                      ('hr')
#define LOBBYAPI_LANGUAGE_CZECH                         ('cs')
#define LOBBYAPI_LANGUAGE_DANISH                        ('da')
#define LOBBYAPI_LANGUAGE_DUTCH                         ('nl')
#define LOBBYAPI_LANGUAGE_ENGLISH                       ('en')
#define LOBBYAPI_LANGUAGE_ESPERANTO                     ('eo')
#define LOBBYAPI_LANGUAGE_ESTONIAN                      ('et')
#define LOBBYAPI_LANGUAGE_FAEROESE                      ('fo')
#define LOBBYAPI_LANGUAGE_FIJI                          ('fj')
#define LOBBYAPI_LANGUAGE_FINNISH                       ('fi')
#define LOBBYAPI_LANGUAGE_FRENCH                        ('fr')
#define LOBBYAPI_LANGUAGE_FRISIAN                       ('fy')
#define LOBBYAPI_LANGUAGE_GALICIAN                      ('gl')
#define LOBBYAPI_LANGUAGE_GEORGIAN                      ('ka')
#define LOBBYAPI_LANGUAGE_GERMAN                        ('de')
#define LOBBYAPI_LANGUAGE_GREEK                         ('el')
#define LOBBYAPI_LANGUAGE_GREENLANDIC                   ('kl')
#define LOBBYAPI_LANGUAGE_GUARANI                       ('gn')
#define LOBBYAPI_LANGUAGE_GUJARATI                      ('gu')
#define LOBBYAPI_LANGUAGE_HAUSA                         ('ha')
#define LOBBYAPI_LANGUAGE_HEBREW                        ('he')
#define LOBBYAPI_LANGUAGE_HINDI                         ('hi')
#define LOBBYAPI_LANGUAGE_HUNGARIAN                     ('hu')
#define LOBBYAPI_LANGUAGE_ICELANDIC                     ('is')
#define LOBBYAPI_LANGUAGE_INDONESIAN                    ('id')
#define LOBBYAPI_LANGUAGE_INTERLINGUA                   ('ia')
#define LOBBYAPI_LANGUAGE_INTERLINGUE                   ('ie')
#define LOBBYAPI_LANGUAGE_INUPIAK                       ('ik')
#define LOBBYAPI_LANGUAGE_INUKTITUT                     ('iu')
#define LOBBYAPI_LANGUAGE_IRISH                         ('ga')
#define LOBBYAPI_LANGUAGE_ITALIAN                       ('it')
#define LOBBYAPI_LANGUAGE_JAPANESE                      ('ja')
#define LOBBYAPI_LANGUAGE_JAVANESE                      ('jw')
#define LOBBYAPI_LANGUAGE_KANNADA                       ('kn')
#define LOBBYAPI_LANGUAGE_KASHMIRI                      ('ks')
#define LOBBYAPI_LANGUAGE_KAZAKH                        ('kk')
#define LOBBYAPI_LANGUAGE_KINYARWANDA                   ('rw')
#define LOBBYAPI_LANGUAGE_KIRGHIZ                       ('ky')
#define LOBBYAPI_LANGUAGE_KIRUNDI                       ('rn')
#define LOBBYAPI_LANGUAGE_KOREAN                        ('ko')
#define LOBBYAPI_LANGUAGE_KURDISH                       ('ku')
#define LOBBYAPI_LANGUAGE_LAOTHIAN                      ('lo')
#define LOBBYAPI_LANGUAGE_LATIN                         ('la')
#define LOBBYAPI_LANGUAGE_LATVIAN_LETTISH               ('lv')
#define LOBBYAPI_LANGUAGE_LINGALA                       ('ln')
#define LOBBYAPI_LANGUAGE_LITHUANIAN                    ('lt')
#define LOBBYAPI_LANGUAGE_MACEDONIAN                    ('mk')
#define LOBBYAPI_LANGUAGE_MALAGASY                      ('mg')
#define LOBBYAPI_LANGUAGE_MALAY                         ('ms')
#define LOBBYAPI_LANGUAGE_MALAYALAM                     ('ml')
#define LOBBYAPI_LANGUAGE_MALTESE                       ('mt')
#define LOBBYAPI_LANGUAGE_MAORI                         ('mi')
#define LOBBYAPI_LANGUAGE_MARATHI                       ('mr')
#define LOBBYAPI_LANGUAGE_MOLDAVIAN                     ('mo')
#define LOBBYAPI_LANGUAGE_MONGOLIAN                     ('mn')
#define LOBBYAPI_LANGUAGE_NAURU                         ('na')
#define LOBBYAPI_LANGUAGE_NEPALI                        ('ne')
#define LOBBYAPI_LANGUAGE_NORWEGIAN                     ('no')
#define LOBBYAPI_LANGUAGE_OCCITAN                       ('oc')
#define LOBBYAPI_LANGUAGE_ORIYA                         ('or')
#define LOBBYAPI_LANGUAGE_PASHTO_PUSHTO                 ('ps')
#define LOBBYAPI_LANGUAGE_PERSIAN                       ('fa')
#define LOBBYAPI_LANGUAGE_POLISH                        ('pl')
#define LOBBYAPI_LANGUAGE_PORTUGUESE                    ('pt')
#define LOBBYAPI_LANGUAGE_PUNJABI                       ('pa')
#define LOBBYAPI_LANGUAGE_QUECHUA                       ('qu')
#define LOBBYAPI_LANGUAGE_RHAETO_ROMANCE                ('rm')
#define LOBBYAPI_LANGUAGE_ROMANIAN                      ('ro')
#define LOBBYAPI_LANGUAGE_RUSSIAN                       ('ru')
#define LOBBYAPI_LANGUAGE_SAMOAN                        ('sm')
#define LOBBYAPI_LANGUAGE_SANGRO                        ('sg')
#define LOBBYAPI_LANGUAGE_SANSKRIT                      ('sa')
#define LOBBYAPI_LANGUAGE_SCOTS_GAELIC                  ('gd')
#define LOBBYAPI_LANGUAGE_SERBIAN                       ('sr')
#define LOBBYAPI_LANGUAGE_SERBO_CROATIAN                ('sh')
#define LOBBYAPI_LANGUAGE_SESOTHO                       ('st')
#define LOBBYAPI_LANGUAGE_SETSWANA                      ('tn')
#define LOBBYAPI_LANGUAGE_SHONA                         ('sn')
#define LOBBYAPI_LANGUAGE_SINDHI                        ('sd')
#define LOBBYAPI_LANGUAGE_SINGHALESE                    ('si')
#define LOBBYAPI_LANGUAGE_SISWATI                       ('ss')
#define LOBBYAPI_LANGUAGE_SLOVAK                        ('sk')
#define LOBBYAPI_LANGUAGE_SLOVENIAN                     ('sl')
#define LOBBYAPI_LANGUAGE_SOMALI                        ('so')
#define LOBBYAPI_LANGUAGE_SPANISH                       ('es')
#define LOBBYAPI_LANGUAGE_SUDANESE                      ('su')
#define LOBBYAPI_LANGUAGE_SWAHILI                       ('sw')
#define LOBBYAPI_LANGUAGE_SWEDISH                       ('sv')
#define LOBBYAPI_LANGUAGE_TAGALOG                       ('tl')
#define LOBBYAPI_LANGUAGE_TAJIK                         ('tg')
#define LOBBYAPI_LANGUAGE_TAMIL                         ('ta')
#define LOBBYAPI_LANGUAGE_TATAR                         ('tt')
#define LOBBYAPI_LANGUAGE_TELUGU                        ('te')
#define LOBBYAPI_LANGUAGE_THAI                          ('th')
#define LOBBYAPI_LANGUAGE_TIBETAN                       ('bo')
#define LOBBYAPI_LANGUAGE_TIGRINYA                      ('ti')
#define LOBBYAPI_LANGUAGE_TONGA                         ('to')
#define LOBBYAPI_LANGUAGE_TSONGA                        ('ts')
#define LOBBYAPI_LANGUAGE_TURKISH                       ('tr')
#define LOBBYAPI_LANGUAGE_TURKMEN                       ('tk')
#define LOBBYAPI_LANGUAGE_TWI                           ('tw')
#define LOBBYAPI_LANGUAGE_UIGHUR                        ('ug')
#define LOBBYAPI_LANGUAGE_UKRAINIAN                     ('uk')
#define LOBBYAPI_LANGUAGE_URDU                          ('ur')
#define LOBBYAPI_LANGUAGE_UZBEK                         ('uz')
#define LOBBYAPI_LANGUAGE_VIETNAMESE                    ('vi')
#define LOBBYAPI_LANGUAGE_VOLAPUK                       ('vo')
#define LOBBYAPI_LANGUAGE_WELSH                         ('cy')
#define LOBBYAPI_LANGUAGE_WOLOF                         ('wo')
#define LOBBYAPI_LANGUAGE_XHOSA                         ('xh')
#define LOBBYAPI_LANGUAGE_YIDDISH                       ('yi')
#define LOBBYAPI_LANGUAGE_YORUBA                        ('yo')
#define LOBBYAPI_LANGUAGE_ZHUANG                        ('za')
#define LOBBYAPI_LANGUAGE_ZULU                          ('zu')

// Languages: added on Mar-25-2011 according to ISO 639-1
#define LOBBYAPI_LANGUAGE_BOSNIAN                       ('bs')
#define LOBBYAPI_LANGUAGE_DIVEHI                        ('dv')
#define LOBBYAPI_LANGUAGE_IGBO                          ('ig')
#define LOBBYAPI_LANGUAGE_LUXEMBOURGISH                 ('lb')
#define LOBBYAPI_LANGUAGE_YI                            ('ii')
#define LOBBYAPI_LANGUAGE_NORWEGIAN_BOKMAL              ('nb')
#define LOBBYAPI_LANGUAGE_NORWEGIAN_NYNORSK             ('nn')
#define LOBBYAPI_LANGUAGE_SAMI                          ('se')

// Default language
#define LOBBYAPI_LANGUAGE_DEFAULT                       LOBBYAPI_LANGUAGE_ENGLISH
#define LOBBYAPI_LANGUAGE_DEFAULT_STR                   "en"

//! Countries
#define LOBBYAPI_COUNTRY_AFGHANISTAN                                        ('AF')
#define LOBBYAPI_COUNTRY_ALBANIA                                            ('AL')
#define LOBBYAPI_COUNTRY_ALGERIA                                            ('DZ')
#define LOBBYAPI_COUNTRY_AMERICAN_SAMOA                                     ('AS')
#define LOBBYAPI_COUNTRY_ANDORRA                                            ('AD')
#define LOBBYAPI_COUNTRY_ANGOLA                                             ('AO')
#define LOBBYAPI_COUNTRY_ANGUILLA                                           ('AI')
#define LOBBYAPI_COUNTRY_ANTARCTICA                                         ('AQ')
#define LOBBYAPI_COUNTRY_ANTIGUA_BARBUDA                                    ('AG')
#define LOBBYAPI_COUNTRY_ARGENTINA                                          ('AR')
#define LOBBYAPI_COUNTRY_ARMENIA                                            ('AM')
#define LOBBYAPI_COUNTRY_ARUBA                                              ('AW')
#define LOBBYAPI_COUNTRY_AUSTRALIA                                          ('AU')
#define LOBBYAPI_COUNTRY_AUSTRIA                                            ('AT')
#define LOBBYAPI_COUNTRY_AZERBAIJAN                                         ('AZ')
#define LOBBYAPI_COUNTRY_BAHAMAS                                            ('BS')
#define LOBBYAPI_COUNTRY_BAHRAIN                                            ('BH')
#define LOBBYAPI_COUNTRY_BANGLADESH                                         ('BD')
#define LOBBYAPI_COUNTRY_BARBADOS                                           ('BB')
#define LOBBYAPI_COUNTRY_BELARUS                                            ('BY')
#define LOBBYAPI_COUNTRY_BELGIUM                                            ('BE')
#define LOBBYAPI_COUNTRY_BELIZE                                             ('BZ')
#define LOBBYAPI_COUNTRY_BENIN                                              ('BJ')
#define LOBBYAPI_COUNTRY_BERMUDA                                            ('BM')
#define LOBBYAPI_COUNTRY_BHUTAN                                             ('BT')
#define LOBBYAPI_COUNTRY_BOLIVIA                                            ('BO')
#define LOBBYAPI_COUNTRY_BOSNIA_HERZEGOVINA                                 ('BA')
#define LOBBYAPI_COUNTRY_BOTSWANA                                           ('BW')
#define LOBBYAPI_COUNTRY_BOUVET_ISLAND                                      ('BV')
#define LOBBYAPI_COUNTRY_BRAZIL                                             ('BR')
#define LOBBYAPI_COUNTRY_BRITISH_INDIAN_OCEAN_TERRITORY                     ('IO')
#define LOBBYAPI_COUNTRY_BRUNEI_DARUSSALAM                                  ('BN')
#define LOBBYAPI_COUNTRY_BULGARIA                                           ('BG')
#define LOBBYAPI_COUNTRY_BURKINA_FASO                                       ('BF')
#define LOBBYAPI_COUNTRY_BURUNDI                                            ('BI')
#define LOBBYAPI_COUNTRY_CAMBODIA                                           ('KH')
#define LOBBYAPI_COUNTRY_CAMEROON                                           ('CM')
#define LOBBYAPI_COUNTRY_CANADA                                             ('CA')
#define LOBBYAPI_COUNTRY_CAPE_VERDE                                         ('CV')
#define LOBBYAPI_COUNTRY_CAYMAN_ISLANDS                                     ('KY')
#define LOBBYAPI_COUNTRY_CENTRAL_AFRICAN_REPUBLIC                           ('CF')
#define LOBBYAPI_COUNTRY_CHAD                                               ('TD')
#define LOBBYAPI_COUNTRY_CHILE                                              ('CL')
#define LOBBYAPI_COUNTRY_CHINA                                              ('CN')
#define LOBBYAPI_COUNTRY_CHRISTMAS_ISLAND                                   ('CX')
#define LOBBYAPI_COUNTRY_COCOS_KEELING_ISLANDS                              ('CC')
#define LOBBYAPI_COUNTRY_COLOMBIA                                           ('CO')
#define LOBBYAPI_COUNTRY_COMOROS                                            ('KM')
#define LOBBYAPI_COUNTRY_CONGO                                              ('CG')
#define LOBBYAPI_COUNTRY_COOK_ISLANDS                                       ('CK')
#define LOBBYAPI_COUNTRY_COSTA_RICA                                         ('CR')
#define LOBBYAPI_COUNTRY_COTE_DIVOIRE                                       ('CI')
#define LOBBYAPI_COUNTRY_CROATIA                                            ('HR')
#define LOBBYAPI_COUNTRY_CUBA                                               ('CU')
#define LOBBYAPI_COUNTRY_CYPRUS                                             ('CY')
#define LOBBYAPI_COUNTRY_CZECH_REPUBLIC                                     ('CZ')
#define LOBBYAPI_COUNTRY_DENMARK                                            ('DK')
#define LOBBYAPI_COUNTRY_DJIBOUTI                                           ('DJ')
#define LOBBYAPI_COUNTRY_DOMINICA                                           ('DM')
#define LOBBYAPI_COUNTRY_DOMINICAN_REPUBLIC                                 ('DO')
#define LOBBYAPI_COUNTRY_EAST_TIMOR                                         ('TP')
#define LOBBYAPI_COUNTRY_ECUADOR                                            ('EC')
#define LOBBYAPI_COUNTRY_EGYPT                                              ('EG')
#define LOBBYAPI_COUNTRY_EL_SALVADOR                                        ('SV')
#define LOBBYAPI_COUNTRY_EQUATORIAL_GUINEA                                  ('GQ')
#define LOBBYAPI_COUNTRY_ERITREA                                            ('ER')
#define LOBBYAPI_COUNTRY_ESTONIA                                            ('EE')
#define LOBBYAPI_COUNTRY_ETHIOPIA                                           ('ET')
#define LOBBYAPI_COUNTRY_EUROPE_SSGFI_ONLY                                  ('EU')
#define LOBBYAPI_COUNTRY_FALKLAND_ISLANDS                                   ('FK')
#define LOBBYAPI_COUNTRY_FAEROE_ISLANDS                                     ('FO')
#define LOBBYAPI_COUNTRY_FIJI                                               ('FJ')
#define LOBBYAPI_COUNTRY_FINLAND                                            ('FI')
#define LOBBYAPI_COUNTRY_FRANCE                                             ('FR')
#define LOBBYAPI_COUNTRY_FRANCE_METROPOLITAN                                ('FX')
#define LOBBYAPI_COUNTRY_FRENCH_GUIANA                                      ('GF')
#define LOBBYAPI_COUNTRY_FRENCH_POLYNESIA                                   ('PF')
#define LOBBYAPI_COUNTRY_FRENCH_SOUTHERN_TERRITORIES                        ('TF')
#define LOBBYAPI_COUNTRY_GABON                                              ('GA')
#define LOBBYAPI_COUNTRY_GAMBIA                                             ('GM')
#define LOBBYAPI_COUNTRY_GEORGIA                                            ('GE')
#define LOBBYAPI_COUNTRY_GERMANY                                            ('DE')
#define LOBBYAPI_COUNTRY_GHANA                                              ('GH')
#define LOBBYAPI_COUNTRY_GIBRALTAR                                          ('GI')
#define LOBBYAPI_COUNTRY_GREECE                                             ('GR')
#define LOBBYAPI_COUNTRY_GREENLAND                                          ('GL')
#define LOBBYAPI_COUNTRY_GRENADA                                            ('GD')
#define LOBBYAPI_COUNTRY_GUADELOUPE                                         ('GP')
#define LOBBYAPI_COUNTRY_GUAM                                               ('GU')
#define LOBBYAPI_COUNTRY_GUATEMALA                                          ('GT')
#define LOBBYAPI_COUNTRY_GUINEA                                             ('GN')
#define LOBBYAPI_COUNTRY_GUINEA_BISSAU                                      ('GW')
#define LOBBYAPI_COUNTRY_GUYANA                                             ('GY')
#define LOBBYAPI_COUNTRY_HAITI                                              ('HT')
#define LOBBYAPI_COUNTRY_HEARD_AND_MC_DONALD_ISLANDS                        ('HM')
#define LOBBYAPI_COUNTRY_HONDURAS                                           ('HN')
#define LOBBYAPI_COUNTRY_HONG_KONG                                          ('HK')
#define LOBBYAPI_COUNTRY_HUNGARY                                            ('HU')
#define LOBBYAPI_COUNTRY_ICELAND                                            ('IS')
#define LOBBYAPI_COUNTRY_INDIA                                              ('IN')
#define LOBBYAPI_COUNTRY_INDONESIA                                          ('ID')
#define LOBBYAPI_COUNTRY_INTERNATIONAL_SSGFI_ONLY                           ('II')
#define LOBBYAPI_COUNTRY_IRAN                                               ('IR')
#define LOBBYAPI_COUNTRY_IRAQ                                               ('IQ')
#define LOBBYAPI_COUNTRY_IRELAND                                            ('IE')
#define LOBBYAPI_COUNTRY_ISRAEL                                             ('IL')
#define LOBBYAPI_COUNTRY_ITALY                                              ('IT')
#define LOBBYAPI_COUNTRY_JAMAICA                                            ('JM')
#define LOBBYAPI_COUNTRY_JAPAN                                              ('JP')
#define LOBBYAPI_COUNTRY_JORDAN                                             ('JO')
#define LOBBYAPI_COUNTRY_KAZAKHSTAN                                         ('KZ')
#define LOBBYAPI_COUNTRY_KENYA                                              ('KE')
#define LOBBYAPI_COUNTRY_KIRIBATI                                           ('KI')
#define LOBBYAPI_COUNTRY_KOREA_DEMOCRATIC_PEOPLES_REPUBLIC_OF               ('KP')
#define LOBBYAPI_COUNTRY_KOREA_REPUBLIC_OF                                  ('KR')
#define LOBBYAPI_COUNTRY_KUWAIT                                             ('KW')
#define LOBBYAPI_COUNTRY_KYRGYZSTAN                                         ('KG')
#define LOBBYAPI_COUNTRY_LAO_PEOPLES_DEMOCRATIC_REPUBLIC                    ('LA')
#define LOBBYAPI_COUNTRY_LATVIA                                             ('LV')
#define LOBBYAPI_COUNTRY_LEBANON                                            ('LB')
#define LOBBYAPI_COUNTRY_LESOTHO                                            ('LS')
#define LOBBYAPI_COUNTRY_LIBERIA                                            ('LR')
#define LOBBYAPI_COUNTRY_LIBYAN_ARAB_JAMAHIRIYA                             ('LY')
#define LOBBYAPI_COUNTRY_LIECHTENSTEIN                                      ('LI')
#define LOBBYAPI_COUNTRY_LITHUANIA                                          ('LT')
#define LOBBYAPI_COUNTRY_LUXEMBOURG                                         ('LU')
#define LOBBYAPI_COUNTRY_MACAU                                              ('MO')
#define LOBBYAPI_COUNTRY_MACEDONIA_THE_FORMER_YUGOSLAV_REPUBLIC_OF          ('MK')
#define LOBBYAPI_COUNTRY_MADAGASCAR                                         ('MG')
#define LOBBYAPI_COUNTRY_MALAWI                                             ('MW')
#define LOBBYAPI_COUNTRY_MALAYSIA                                           ('MY')
#define LOBBYAPI_COUNTRY_MALDIVES                                           ('MV')
#define LOBBYAPI_COUNTRY_MALI                                               ('ML')
#define LOBBYAPI_COUNTRY_MALTA                                              ('MT')
#define LOBBYAPI_COUNTRY_MARSHALL_ISLANDS                                   ('MH')
#define LOBBYAPI_COUNTRY_MARTINIQUE                                         ('MQ')
#define LOBBYAPI_COUNTRY_MAURITANIA                                         ('MR')
#define LOBBYAPI_COUNTRY_MAURITIUS                                          ('MU')
#define LOBBYAPI_COUNTRY_MAYOTTE                                            ('YT')
#define LOBBYAPI_COUNTRY_MEXICO                                             ('MX')
#define LOBBYAPI_COUNTRY_MICRONESIA_FEDERATED_STATES_OF                     ('FM')
#define LOBBYAPI_COUNTRY_MOLDOVA_REPUBLIC_OF                                ('MD')
#define LOBBYAPI_COUNTRY_MONACO                                             ('MC')
#define LOBBYAPI_COUNTRY_MONGOLIA                                           ('MN')
#define LOBBYAPI_COUNTRY_MONTSERRAT                                         ('MS')
#define LOBBYAPI_COUNTRY_MOROCCO                                            ('MA')
#define LOBBYAPI_COUNTRY_MOZAMBIQUE                                         ('MZ')
#define LOBBYAPI_COUNTRY_MYANMAR                                            ('MM')
#define LOBBYAPI_COUNTRY_NAMIBIA                                            ('NA')
#define LOBBYAPI_COUNTRY_NAURU                                              ('NR')
#define LOBBYAPI_COUNTRY_NEPAL                                              ('NP')
#define LOBBYAPI_COUNTRY_NETHERLANDS                                        ('NL')
#define LOBBYAPI_COUNTRY_NETHERLANDS_ANTILLES                               ('AN')
#define LOBBYAPI_COUNTRY_NEW_CALEDONIA                                      ('NC')
#define LOBBYAPI_COUNTRY_NEW_ZEALAND                                        ('NZ')
#define LOBBYAPI_COUNTRY_NICARAGUA                                          ('NI')
#define LOBBYAPI_COUNTRY_NIGER                                              ('NE')
#define LOBBYAPI_COUNTRY_NIGERIA                                            ('NG')
#define LOBBYAPI_COUNTRY_NIUE                                               ('NU')
#define LOBBYAPI_COUNTRY_NORFOLK_ISLAND                                     ('NF')
#define LOBBYAPI_COUNTRY_NORTHERN_MARIANA_ISLANDS                           ('MP')
#define LOBBYAPI_COUNTRY_NORWAY                                             ('NO')
#define LOBBYAPI_COUNTRY_OMAN                                               ('OM')
#define LOBBYAPI_COUNTRY_PAKISTAN                                           ('PK')
#define LOBBYAPI_COUNTRY_PALAU                                              ('PW')
#define LOBBYAPI_COUNTRY_PANAMA                                             ('PA')
#define LOBBYAPI_COUNTRY_PAPUA_NEW_GUINEA                                   ('PG')
#define LOBBYAPI_COUNTRY_PARAGUAY                                           ('PY')
#define LOBBYAPI_COUNTRY_PERU                                               ('PE')
#define LOBBYAPI_COUNTRY_PHILIPPINES                                        ('PH')
#define LOBBYAPI_COUNTRY_PITCAIRN                                           ('PN')
#define LOBBYAPI_COUNTRY_POLAND                                             ('PL')
#define LOBBYAPI_COUNTRY_PORTUGAL                                           ('PT')
#define LOBBYAPI_COUNTRY_PUERTO_RICO                                        ('PR')
#define LOBBYAPI_COUNTRY_QATAR                                              ('QA')
#define LOBBYAPI_COUNTRY_REUNION                                            ('RE')
#define LOBBYAPI_COUNTRY_ROMANIA                                            ('RO')
#define LOBBYAPI_COUNTRY_RUSSIAN_FEDERATION                                 ('RU')
#define LOBBYAPI_COUNTRY_RWANDA                                             ('RW')
#define LOBBYAPI_COUNTRY_SAINT_KITTS_AND_NEVIS                              ('KN')
#define LOBBYAPI_COUNTRY_SAINT_LUCIA                                        ('LC')
#define LOBBYAPI_COUNTRY_SAINT_VINCENT_AND_THE_GRENADINES                   ('VC')
#define LOBBYAPI_COUNTRY_SAMOA                                              ('WS')
#define LOBBYAPI_COUNTRY_SAN_MARINO                                         ('SM')
#define LOBBYAPI_COUNTRY_SAO_TOME_AND_PRINCIPE                              ('ST')
#define LOBBYAPI_COUNTRY_SAUDI_ARABIA                                       ('SA')
#define LOBBYAPI_COUNTRY_SENEGAL                                            ('SN')
#define LOBBYAPI_COUNTRY_SEYCHELLES                                         ('SC')
#define LOBBYAPI_COUNTRY_SIERRA_LEONE                                       ('SL')
#define LOBBYAPI_COUNTRY_SINGAPORE                                          ('SG')
#define LOBBYAPI_COUNTRY_SLOVAKIA                                           ('SK')
#define LOBBYAPI_COUNTRY_SLOVENIA                                           ('SI')
#define LOBBYAPI_COUNTRY_SOLOMON_ISLANDS                                    ('SB')
#define LOBBYAPI_COUNTRY_SOMALIA                                            ('SO')
#define LOBBYAPI_COUNTRY_SOUTH_AFRICA                                       ('ZA')
#define LOBBYAPI_COUNTRY_SOUTH_GEORGIA_AND_THE_SOUTH_SANDWICH_ISLANDS       ('GS')
#define LOBBYAPI_COUNTRY_SPAIN                                              ('ES')
#define LOBBYAPI_COUNTRY_SRI_LANKA                                          ('LK')
#define LOBBYAPI_COUNTRY_ST_HELENA_ASCENSION_AND_TRISTAN_DA_CUNHA           ('SH')
#define LOBBYAPI_COUNTRY_ST_PIERRE_AND_MIQUELON                             ('PM')
#define LOBBYAPI_COUNTRY_SUDAN                                              ('SD')
#define LOBBYAPI_COUNTRY_SURINAME                                           ('SR')
#define LOBBYAPI_COUNTRY_SVALBARD_AND_JAN_MAYEN_ISLANDS                     ('SJ')
#define LOBBYAPI_COUNTRY_SWAZILAND                                          ('SZ')
#define LOBBYAPI_COUNTRY_SWEDEN                                             ('SE')
#define LOBBYAPI_COUNTRY_SWITZERLAND                                        ('CH')
#define LOBBYAPI_COUNTRY_SYRIAN_ARAB_REPUBLIC                               ('SY')
#define LOBBYAPI_COUNTRY_TAIWAN                                             ('TW')
#define LOBBYAPI_COUNTRY_TAJIKISTAN                                         ('TJ')
#define LOBBYAPI_COUNTRY_TANZANIA_UNITED_REPUBLIC_OF                        ('TZ')
#define LOBBYAPI_COUNTRY_THAILAND                                           ('TH')
#define LOBBYAPI_COUNTRY_TOGO                                               ('TG')
#define LOBBYAPI_COUNTRY_TOKELAU                                            ('TK')
#define LOBBYAPI_COUNTRY_TONGA                                              ('TO')
#define LOBBYAPI_COUNTRY_TRINIDAD_AND_TOBAGO                                ('TT')
#define LOBBYAPI_COUNTRY_TUNISIA                                            ('TN')
#define LOBBYAPI_COUNTRY_TURKEY                                             ('TR')
#define LOBBYAPI_COUNTRY_TURKMENISTAN                                       ('TM')
#define LOBBYAPI_COUNTRY_TURKS_AND_CAICOS_ISLANDS                           ('TC')
#define LOBBYAPI_COUNTRY_TUVALU                                             ('TV')
#define LOBBYAPI_COUNTRY_UGANDA                                             ('UG')
#define LOBBYAPI_COUNTRY_UKRAINE                                            ('UA')
#define LOBBYAPI_COUNTRY_UNITED_ARAB_EMIRATES                               ('AE')
#define LOBBYAPI_COUNTRY_UNITED_KINGDOM                                     ('GB')
#define LOBBYAPI_COUNTRY_UNITED_STATES                                      ('US')
#define LOBBYAPI_COUNTRY_UNITED_STATES_MINOR_OUTLYING_ISLANDS               ('UM')
#define LOBBYAPI_COUNTRY_URUGUAY                                            ('UY')
#define LOBBYAPI_COUNTRY_UZBEKISTAN                                         ('UZ')
#define LOBBYAPI_COUNTRY_VANUATU                                            ('VU')
#define LOBBYAPI_COUNTRY_VATICAN_CITY_STATE                                 ('VA')
#define LOBBYAPI_COUNTRY_VENEZUELA                                          ('VE')
#define LOBBYAPI_COUNTRY_VIETNAM                                            ('VN')
#define LOBBYAPI_COUNTRY_VIRGIN_ISLANDS_BRITISH                             ('VG')
#define LOBBYAPI_COUNTRY_VIRGIN_ISLANDS_US                                  ('VI')
#define LOBBYAPI_COUNTRY_WALLIS_AND_FUTUNA_ISLANDS                          ('WF')
#define LOBBYAPI_COUNTRY_WESTERN_SAHARA                                     ('EH')
#define LOBBYAPI_COUNTRY_YEMEN                                              ('YE')
#define LOBBYAPI_COUNTRY_YUGOSLAVIA                                         ('YU')
#define LOBBYAPI_COUNTRY_ZAIRE                                              ('ZR')
#define LOBBYAPI_COUNTRY_ZAMBIA                                             ('ZM')
#define LOBBYAPI_COUNTRY_ZIMBABWE                                           ('ZW')

// Countries: added on Mar-25-2011
#define LOBBYAPI_COUNTRY_SERBIA_AND_MONTENEGRO                              ('CS')
#define LOBBYAPI_COUNTRY_MONTENEGRO                                         ('ME')
#define LOBBYAPI_COUNTRY_SERBIA                                             ('RS')
#define LOBBYAPI_COUNTRY_CONGO_DRC                                          ('CD')
#define LOBBYAPI_COUNTRY_PALESTINIAN_TERRITORY                              ('PS')
#define LOBBYAPI_COUNTRY_GUERNSEY                                           ('GG')
#define LOBBYAPI_COUNTRY_JERSEY                                             ('JE')
#define LOBBYAPI_COUNTRY_ISLE_OF_MAN                                        ('IM')
#define LOBBYAPI_COUNTRY_TIMOR_LESTE                                        ('TL')

// Default country
#define LOBBYAPI_COUNTRY_DEFAULT                                            LOBBYAPI_COUNTRY_UNITED_STATES
#define LOBBYAPI_COUNTRY_DEFAULT_STR                                        "US"


//! Currencies   (ISO-4217)
#define LOBBYAPI_CURRENCY_UNITED_ARAB_EMIRATS_DIRHAM                        ('AED')
#define LOBBYAPI_CURRENCY_AFGHAN_AFGHANI                                    ('AFN')
#define LOBBYAPI_CURRENCY_ALBANIAN_LEK                                      ('ALL')
#define LOBBYAPI_CURRENCY_ARMENIAN_DRAM                                     ('AMD')
#define LOBBYAPI_CURRENCY_NETHERLANDS_ANTILLEAN_GUILDER                     ('ANG')
#define LOBBYAPI_CURRENCY_ANGOLAN_KWANZA                                    ('AOA')
#define LOBBYAPI_CURRENCY_ARGENTINE_PESO                                    ('ARS')
#define LOBBYAPI_CURRENCY_AUSTRALIAN_DOLLAR                                 ('AUD')
#define LOBBYAPI_CURRENCY_ARUBAN_FLORIN                                     ('AWG')
#define LOBBYAPI_CURRENCY_AZERBAIJANI_PMANAT                                ('AZN')
#define LOBBYAPI_CURRENCY_BOSNIA_AND_HERZEGOVINA_CONVERTIBLE_MARK           ('BAM')
#define LOBBYAPI_CURRENCY_BARBADOS_DOLLAR                                   ('BBD')
#define LOBBYAPI_CURRENCY_BANGLADESHI_TAKA                                  ('BDT')
#define LOBBYAPI_CURRENCY_BULGARIAN_LEV                                     ('BGN')
#define LOBBYAPI_CURRENCY_BAHRAINI_DINAR                                    ('BHD')
#define LOBBYAPI_CURRENCY_BURUNDIAN_FRANC                                   ('BIF')
#define LOBBYAPI_CURRENCY_BERMUDIAN_DOLLAR                                  ('BMD')
#define LOBBYAPI_CURRENCY_BRUNEI_DOLLAR                                     ('BND')
#define LOBBYAPI_CURRENCY_BOLIVIANO                                         ('BOB')
#define LOBBYAPI_CURRENCY_BOLIVIAN_MVDOL                                    ('BOV')
#define LOBBYAPI_CURRENCY_BRAZILIAN_REAL                                    ('BRL')
#define LOBBYAPI_CURRENCY_BAHAMIAN_DOLLAR                                   ('BSD')
#define LOBBYAPI_CURRENCY_BHUTANESE_NGULTRUM                                ('BTN')
#define LOBBYAPI_CURRENCY_BOTSWANA_PULA                                     ('BWP')
#define LOBBYAPI_CURRENCY_BELARUSIAN_RUBLE                                  ('BYR')
#define LOBBYAPI_CURRENCY_BELIZE_DOLLAR                                     ('BZD')
#define LOBBYAPI_CURRENCY_CANADIAN_DOLLAR                                   ('CAD')
#define LOBBYAPI_CURRENCY_CONGOLESE_FRANC                                   ('CDF')
#define LOBBYAPI_CURRENCY_WIR_EURO                                          ('CHE')
#define LOBBYAPI_CURRENCY_SWISS_FRANC                                       ('CHF')
#define LOBBYAPI_CURRENCY_WIR_FRANC                                         ('CHW')
#define LOBBYAPI_CURRENCY_UNIDAD_DE_FOMENTO                                 ('CLF')
#define LOBBYAPI_CURRENCY_CHILEAN_PESO                                      ('CLP')
#define LOBBYAPI_CURRENCY_CHINESE_YUAN                                      ('CNY')
#define LOBBYAPI_CURRENCY_COLOBIAN_PESO                                     ('COP')
#define LOBBYAPI_CURRENCY_UNIDAD_DE_VALOR_REAL                              ('COU')
#define LOBBYAPI_CURRENCY_COSTA_RICAN_COLON                                 ('CRC')
#define LOBBYAPI_CURRENCY_CUBAN_CONVERTIBLE_PESO                            ('CUC')
#define LOBBYAPI_CURRENCY_CUBAN_PESO                                        ('CUP')
#define LOBBYAPI_CURRENCY_CAP_VERDE_ESCUDO                                  ('CVE')
#define LOBBYAPI_CURRENCY_CZECH_KORUNA                                      ('CZK')
#define LOBBYAPI_CURRENCY_DJIBOUTIAN_FRANC                                  ('DJF')
#define LOBBYAPI_CURRENCY_DANISH_KRONE                                      ('DKK')
#define LOBBYAPI_CURRENCY_DOMINICAN_PESO                                    ('DOP')
#define LOBBYAPI_CURRENCY_ALGERIAN_DINAR                                    ('DZD')
#define LOBBYAPI_CURRENCY_EGYPTIAN_POUND                                    ('EGP')
#define LOBBYAPI_CURRENCY_ERITREAN_NAKFA                                    ('ERN')
#define LOBBYAPI_CURRENCY_ETHIOPIAN_BIRR                                    ('ETB')
#define LOBBYAPI_CURRENCY_EURO                                              ('EUR')
#define LOBBYAPI_CURRENCY_FIJI_DOLLAR                                       ('FJD')
#define LOBBYAPI_CURRENCY_FALKLAND_ISLANDS_POUND                            ('FKP')
#define LOBBYAPI_CURRENCY_POUND_STERLING                                    ('GBP')
#define LOBBYAPI_CURRENCY_GEORGIAN_LARI                                     ('GEL')
#define LOBBYAPI_CURRENCY_GHANAIAN_CEDI                                     ('GHS')
#define LOBBYAPI_CURRENCY_GIBRALTAR_POUND                                   ('GIP')
#define LOBBYAPI_CURRENCY_GAMBIAN_DALASI                                    ('GMD')
#define LOBBYAPI_CURRENCY_GUINEAN_FRANC                                     ('GNF')
#define LOBBYAPI_CURRENCY_GUATEMALAN_QUETZAL                                ('GTQ')
#define LOBBYAPI_CURRENCY_GUYANESE_DOLLAR                                   ('GYD')
#define LOBBYAPI_CURRENCY_HONG_KONG_DOLLAR                                  ('HKD')
#define LOBBYAPI_CURRENCY_HONDURAN_LEMPIRA                                  ('HNL')
#define LOBBYAPI_CURRENCY_CROATIAN_KUNA                                     ('HRK')
#define LOBBYAPI_CURRENCY_HAITIAN_GOURDE                                    ('HTG')
#define LOBBYAPI_CURRENCY_HUNGARIAN_FORINT                                  ('HUF')
#define LOBBYAPI_CURRENCY_INDONESIAN_RUPIAH                                 ('IDR')
#define LOBBYAPI_CURRENCY_ISRAELI_NEW_SHEQEL                                ('ILS')
#define LOBBYAPI_CURRENCY_INDIAN_RUPEE                                      ('INR')
#define LOBBYAPI_CURRENCY_IRAQI_DINAR                                       ('IQD')
#define LOBBYAPI_CURRENCY_IRANIAN_RIAL                                      ('IRR')
#define LOBBYAPI_CURRENCY_ICELANDIC_KRONA                                   ('ISK')
#define LOBBYAPI_CURRENCY_JAMAICAN_DOLLAR                                   ('JMD')
#define LOBBYAPI_CURRENCY_JORDANIAN_DINAR                                   ('JOD')
#define LOBBYAPI_CURRENCY_JAPANESE_YEN                                      ('JPY')
#define LOBBYAPI_CURRENCY_KENYAN_SHILLING                                   ('KES')
#define LOBBYAPI_CURRENCY_KYRGYZSTANI_SOM                                   ('KGS')
#define LOBBYAPI_CURRENCY_CAMBODIAN_RIEL                                    ('KHR')
#define LOBBYAPI_CURRENCY_COMORO_FRANC                                      ('KMF')
#define LOBBYAPI_CURRENCY_NORTH_KOREAN_WON                                  ('KPW')
#define LOBBYAPI_CURRENCY_SOUTH_KOREAN_WON                                  ('KRW')
#define LOBBYAPI_CURRENCY_KUWAITI_DINAR                                     ('KWD')
#define LOBBYAPI_CURRENCY_CAYMAN_ISLANDS_DOLLAR                             ('KYD')
#define LOBBYAPI_CURRENCY_KAZAKHSTANI_TENGE                                 ('KZT')
#define LOBBYAPI_CURRENCY_LAO_KIP                                           ('LAK')
#define LOBBYAPI_CURRENCY_LEBANESE_POUND                                    ('LBP')
#define LOBBYAPI_CURRENCY_SRI_LANKAN_RUPEE                                  ('LKR')
#define LOBBYAPI_CURRENCY_LIBERIAN_DOLLAR                                   ('LRD')
#define LOBBYAPI_CURRENCY_LESOTHO_LOTI                                      ('LSL')
#define LOBBYAPI_CURRENCY_LITHUANIAN_LITAS                                  ('LTL')
#define LOBBYAPI_CURRENCY_LATVIAN_LATS                                      ('LVL')
#define LOBBYAPI_CURRENCY_LYBIAN_DINAR                                      ('LYD')
#define LOBBYAPI_CURRENCY_MOROCCAN_DIRHAM                                   ('MAD')
#define LOBBYAPI_CURRENCY_MOLDOVAN_LEU                                      ('MDL')
#define LOBBYAPI_CURRENCY_MALAGASY_ARIARY                                   ('MGA')
#define LOBBYAPI_CURRENCY_MACEDONIAN_DENAR                                  ('MKD')
#define LOBBYAPI_CURRENCY_MYANMA_KYAT                                       ('MMK')
#define LOBBYAPI_CURRENCY_MONGOLIAN_TUGRIK                                  ('MNT')
#define LOBBYAPI_CURRENCY_MACANESE_PATACA                                   ('MOP')
#define LOBBYAPI_CURRENCY_MAURITANIAN_OUGUIYA                               ('MRO')
#define LOBBYAPI_CURRENCY_MAURITIAN_RUPEE                                   ('MUR')
#define LOBBYAPI_CURRENCY_MALDIVIAN_RUFIYAA                                 ('MVR')
#define LOBBYAPI_CURRENCY_MALAWAIAN_KWACHA                                  ('MWK')
#define LOBBYAPI_CURRENCY_MEXICAN_PESO                                      ('MXN')
#define LOBBYAPI_CURRENCY_MEXICAN_UNIDAD_DE_INVERSION                       ('MXV')
#define LOBBYAPI_CURRENCY_MALAYSIAN_RINGGIT                                 ('MYR')
#define LOBBYAPI_CURRENCY_MOZAMBICAN_METICAL                                ('MZN')
#define LOBBYAPI_CURRENCY_NAMIBIAN_DOLLAR                                   ('NAD')
#define LOBBYAPI_CURRENCY_NIGERIAN_NAIRA                                    ('NGN')
#define LOBBYAPI_CURRENCY_NICARAGUAN_CORDOBA                                ('NIO')
#define LOBBYAPI_CURRENCY_NORVEGIAN_KRONE                                   ('NOK')
#define LOBBYAPI_CURRENCY_NEPALESE_RUPEE                                    ('NPR')
#define LOBBYAPI_CURRENCY_NEW_ZEALAND_DOLLAR                                ('NZD')
#define LOBBYAPI_CURRENCY_OMANI_RIAL                                        ('OMR')
#define LOBBYAPI_CURRENCY_PANAMANIAN_BALBOA                                 ('PAB')
#define LOBBYAPI_CURRENCY_PERUVIAN_NUEVO_SOL                                ('PEN')
#define LOBBYAPI_CURRENCY_PAPUA_NEW_GUINEAN_KINA                            ('PGK')
#define LOBBYAPI_CURRENCY_PHILIPPINE_PESO                                   ('PHP')
#define LOBBYAPI_CURRENCY_PAKISTANI_RUPEE                                   ('PKR')
#define LOBBYAPI_CURRENCY_POLISH_ZLOTY                                      ('PLN')
#define LOBBYAPI_CURRENCY_PARAGUAYAN_GUARANI                                ('PYG')
#define LOBBYAPI_CURRENCY_QATARI_RIAL                                       ('QAR')
#define LOBBYAPI_CURRENCY_ROMANIAN_NEW_LEU                                  ('RON')
#define LOBBYAPI_CURRENCY_SERBIAN_DINAR                                     ('RSD')
#define LOBBYAPI_CURRENCY_RUSSIAN_RUBLE                                     ('RUB')
#define LOBBYAPI_CURRENCY_RWANDAN_FRANC                                     ('RWF')
#define LOBBYAPI_CURRENCY_SAUDI_RIYAL                                       ('SAR')
#define LOBBYAPI_CURRENCY_SOLOMON_ISLANDS_DOLLAR                            ('SBD')
#define LOBBYAPI_CURRENCY_SEYCHELLES_RUPEE                                  ('SRC')
#define LOBBYAPI_CURRENCY_SUDANESE_POUND                                    ('SDG')
#define LOBBYAPI_CURRENCY_SWEDISH_KRONA                                     ('SEK')
#define LOBBYAPI_CURRENCY_SINGAPORE_DOLLAR                                  ('SGD')
#define LOBBYAPI_CURRENCY_SAINT_HELENA_POUND                                ('SHP')
#define LOBBYAPI_CURRENCY_SIERRA_LEONEAN_LEONE                              ('SLL')
#define LOBBYAPI_CURRENCY_SOMALI_SHILLING                                   ('SOS')
#define LOBBYAPI_CURRENCY_SURINAMESE_DOLLAR                                 ('SRD')
#define LOBBYAPI_CURRENCY_SOUTH_SUDANESE_POUND                              ('SSP')
#define LOBBYAPI_CURRENCY_SAO_TOME_AND_PRINCIPE_DOBRA                       ('STD')
#define LOBBYAPI_CURRENCY_SYRIAN_POUND                                      ('SYP')
#define LOBBYAPI_CURRENCY_SWAZI_LILANGENI                                   ('SZL')
#define LOBBYAPI_CURRENCY_THAI_BAHT                                         ('THB')
#define LOBBYAPI_CURRENCY_TAJIKISTANI_SOMONI                                ('TJS')
#define LOBBYAPI_CURRENCY_TURKMENISTANI_MANAT                               ('TMT')
#define LOBBYAPI_CURRENCY_TUNISIAN_DINAR                                    ('TND')
#define LOBBYAPI_CURRENCY_TONGAN_PAANGA                                     ('TOP')
#define LOBBYAPI_CURRENCY_TURKISH_LIRA                                      ('TRY')
#define LOBBYAPI_CURRENCY_TRINIDAD_AND_TOBAGO_DOLLAR                        ('TTD')
#define LOBBYAPI_CURRENCY_NEW_TAIWAN_DOLLAR                                 ('TWD')
#define LOBBYAPI_CURRENCY_TANZANIAN_SHILLING                                ('TZS')
#define LOBBYAPI_CURRENCY_UKRAINIAN_HRYVNIA                                 ('UAH')
#define LOBBYAPI_CURRENCY_UGANDAN_SHILLING                                  ('UGX')
#define LOBBYAPI_CURRENCY_UNITED_STATES_DOLLAR                              ('USD')
#define LOBBYAPI_CURRENCY_UNITED_STATES_DOLLAR_NEXT_DAY                     ('USN')
#define LOBBYAPI_CURRENCY_UNITED_STATES_DOLLAR_SAME_DAY                     ('USS')
#define LOBBYAPI_CURRENCY_URUGUAY_PESO_EN_UNIDADES_INDEXADAS                ('UYI')
#define LOBBYAPI_CURRENCY_URUGUAYAN_PESO                                    ('UYU')
#define LOBBYAPI_CURRENCY_UZBEKISTAN_SOM                                    ('UZS')
#define LOBBYAPI_CURRENCY_VENEZUELAN_BOLIVAR_FUERTE                         ('VEF')
#define LOBBYAPI_CURRENCY_VIETNAMESE_DONG                                   ('VND')
#define LOBBYAPI_CURRENCY_VANUATU_VATU                                      ('VUV')
#define LOBBYAPI_CURRENCY_SAMOAN_TALA                                       ('WST')
#define LOBBYAPI_CURRENCY_CFA_FRANC_BEAC                                    ('XAF')
#define LOBBYAPI_CURRENCY_EAST_CARABBEAN_DOLLAR                             ('XCD')
#define LOBBYAPI_CURRENCY_CFA_FRANC_BCEAO                                   ('XOF')
#define LOBBYAPI_CURRENCY_CFP_FRANC                                         ('XPF')
#define LOBBYAPI_CURRENCY_YEMENI_RIAL                                       ('YER')
#define LOBBYAPI_CURRENCY_SOUTH_AFRICAN_RAND                                ('ZAR')
#define LOBBYAPI_CURRENCY_ZAMBIAN_KWACHA                                    ('ZMK')
#define LOBBYAPI_CURRENCY_ZIMBABWE_DOLLAR                                   ('ZWL')

// Default currency
#define LOBBYAPI_CURRENCY_DEFAULT                                           LOBBYAPI_CURRENCY_UNITED_STATES_DOLLAR
#define LOBBYAPI_CURRENCY_DEFAULT_STR                                       "USD"


/*** Macros ***********************************************************************/

//! Write the currency code to a character string
#define LOBBYAPI_CreateCurrencyString(strOutstring, uCurrency)  \
    {   \
        (strOutstring)[0] = (char)(((uCurrency) >> 16 ) & 0xFF); \
        (strOutstring)[1] = (char)(((uCurrency) >> 8) & 0xFF); \
        (strOutstring)[2] = (char)((uCurrency) & 0xFF); \
        (strOutstring)[3]='\0'; \
    }

//! toupper replacement
#define LOBBYAPI_LocalizerTokenToUpper(uCharToModify)                                       \
    ((((unsigned char)(uCharToModify) >= 'a') && ((unsigned char)(uCharToModify) <= 'z')) ? \
            (((unsigned char)(uCharToModify)) & (~32)) : \
            (uCharToModify))

//! tolower replacement
#define LOBBYAPI_LocalizerTokenToLower(uCharToModify)                                       \
    ((((unsigned char)(uCharToModify) >= 'A') && ((unsigned char)(uCharToModify) <= 'Z')) ? \
            (((unsigned char)(uCharToModify)) | (32)) :                                     \
            (uCharToModify))

//! Create a localizer token from shorts representing country and language
#define LOBBYAPI_LocalizerTokenCreate(uLanguage, uCountry)  \
    (((LOBBYAPI_LocalizerTokenShortToLower(uLanguage)) << 16) + (LOBBYAPI_LocalizerTokenShortToUpper(uCountry)))

//! Create a localizer token from strings containing country and language
#define LOBBYAPI_LocalizerTokenCreateFromStrings(strLanguage, strCountry) \
    (LOBBYAPI_LocalizerTokenCreate(LOBBYAPI_LocalizerTokenGetShortFromString(strLanguage),LOBBYAPI_LocalizerTokenGetShortFromString(strCountry)))

//! Create a localizer token from a single string (language + country - ex: "enUS")
#define LOBBYAPI_LocalizerTokenCreateFromString(strLocality) \
    (LOBBYAPI_LocalizerTokenCreateFromStrings(&(strLocality)[0], &(strLocality)[2]))

//! Get a int16_t integer from a string
#define LOBBYAPI_LocalizerTokenGetShortFromString(strInstring) (( (((unsigned char*)(strInstring) == NULL) || ((unsigned char*)strInstring)[0] == '\0')) ? \
                        ((uint16_t)(0)) : \
                        ((uint16_t)((((unsigned char*)strInstring)[0] << 8) | ((unsigned char*)strInstring)[1])))

//! Pull the country (int16_t) from a localizer token (int32_t)
#define LOBBYAPI_LocalizerTokenGetCountry(uToken)           ((uint16_t)((uToken) & 0xFFFF))

//! Pull the language (int16_t) from a localizer token (int32_t)
#define LOBBYAPI_LocalizerTokenGetLanguage(uToken)          ((uint16_t)(((uToken) >> 16) & 0xFFFF))

//! Replace the country in a locality value
#define LOBBYAPI_LocalizerTokenSetCountry(uToken, uCountry)    (uToken) = (((uToken) & 0xFFFF0000) | (uCountry));

//! Replace the language in a locality value
#define LOBBYAPI_LocalizerTokenSetLanguage(uToken, uLanguage)  (uToken) = (((uToken) & 0x0000FFFF) | ((uLanguage) << 16));

//! Write the country contained in a localizer token to a character string
#define LOBBYAPI_LocalizerTokenCreateCountryString(strOutstring, uToken)  \
    {   \
        (strOutstring)[0] = (char)(((uToken) >> 8) & 0xFF); \
        (strOutstring)[1] = (char)((uToken) & 0xFF); \
        (strOutstring)[2]='\0'; \
    }


//! Write the language contained in a localizer token to a character string
#define LOBBYAPI_LocalizerTokenCreateLanguageString(strOutstring, uToken)  \
    {   \
        (strOutstring)[0]=(char)(((uToken) >> 24) & 0xFF); \
        (strOutstring)[1]=(char)(((uToken) >> 16) & 0xFF); \
        (strOutstring)[2]='\0'; \
    }

//! Write the entire locality string to a character string
#define LOBBYAPI_LocalizerTokenCreateLocalityString(strOutstring, uToken)  \
    {   \
        (strOutstring)[0]=(char)(((uToken) >> 24) & 0xFF); \
        (strOutstring)[1]=(char)(((uToken) >> 16) & 0xFF); \
        (strOutstring)[2]=(char)(((uToken) >> 8) & 0xFF); \
        (strOutstring)[3]=(char)((uToken) & 0xFF); \
        (strOutstring)[4]='\0'; \
    }

//! Macro to provide an easy way to display the token in character format
#define LOBBYAPI_LocalizerTokenPrintCharArray(uToken)  \
    (char)(((uToken)>>24)&0xFF), (char)(((uToken)>>16)&0xFF), (char)(((uToken)>>8)&0xFF), (char)((uToken)&0xFF)

//! Provide a way to capitalize the elements in a int16_t
#define LOBBYAPI_LocalizerTokenShortToUpper(uShort)  \
    ((uint16_t)(((LOBBYAPI_LocalizerTokenToUpper(((uShort) >> 8) & 0xFF)) << 8) + \
                       (LOBBYAPI_LocalizerTokenToUpper((uShort) & 0xFF))))

//! Provide a way to lowercase the elements in a int16_t
#define LOBBYAPI_LocalizerTokenShortToLower(uShort)  \
    ((uint16_t)(((LOBBYAPI_LocalizerTokenToLower(((uShort) >> 8) & 0xFF)) << 8) + \
                       (LOBBYAPI_LocalizerTokenToLower((uShort) & 0xFF))))

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

//@}

#endif // _dirtylang_h

