#include "dataBase.h"

map<string, srcFunction> srcKeywords = { {"author", searchAuthor}, {"genre", searchGenre},
                                                {"subgenre", searchSubgenre}, {"title", searchTitle},
                                                {"ISBN", searchISBN}, {"year", searchYear}
                                              };


sqlite3 *myDataBase;

bool existsUser(const char* usrname) {
    int rc;
    sqlite3_stmt *stmt;
    const char *sql =   "SELECT * FROM USERS " \
                        "WHERE UPPER(USERNAME) = UPPER(?);";

    rc = sqlite3_prepare_v2(myDataBase, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot prepare statement.\n";
        return 0;
    }

    rc = sqlite3_bind_text(stmt, 1, usrname, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot bind parameter.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        std::cout<<"Cannot run querry.\n";
        sqlite3_finalize(stmt);
        return 0;    
    }

    sqlite3_finalize(stmt);
    return (rc != SQLITE_DONE);
}

bool isPasswdCorrect(const char* usrname, const char* passwd) {
    int rc;
    sqlite3_stmt *stmt;
    const char *sql =   "SELECT PASSWORD FROM USERS " \
                        "WHERE UPPER(USERNAME) = UPPER(?);";

    rc = sqlite3_prepare_v2(myDataBase, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot prepare statement.\n";
        return 0;
    }

    rc = sqlite3_bind_text(stmt, 1, usrname, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot bind parameter.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        std::cout<<"Cannot run querry.\n";
        sqlite3_finalize(stmt);
        return 0;    
    }

    char decoded[30] = "";
    decodePasswd((const char*)sqlite3_column_text(stmt, 0), decoded);

    sqlite3_finalize(stmt);
    return (strcmp(passwd, decoded) == 0);
}

void decodePasswd(const char* encoded, char* decoded) {
    int i;

    for (i = 0; encoded[i]; i++)
        decoded[i] = (256 + encoded[i] - OFFSET) % 256;

    decoded[i] = 0;
}

void encodePasswd(const char* decoded, char* encoded) {
    int i;

    for (i = 0; decoded[i]; i++)
        encoded[i] = (decoded[i] + OFFSET) % 256;

    encoded[i] = 0;
}

int getUserId(const char* usrname) {
    int rc;
    sqlite3_stmt *stmt;
    const char *sql =   "SELECT ID FROM USERS " \
                        "WHERE UPPER(USERNAME) = UPPER(?);";

    rc = sqlite3_prepare_v2(myDataBase, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot prepare statement.\n";
        return 0;
    }

    rc = sqlite3_bind_text(stmt, 1, usrname, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot bind parameter.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        std::cout<<"Cannot run querry.\n";
        sqlite3_finalize(stmt);
        return 0;    
    }

    int userId = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);
    return userId;
}

bool insertUser(const char* usrname, const char* passwd) {
    int rc;
    sqlite3_stmt *stmt;
    const char *sql =   "INSERT INTO USERS (USERNAME, PASSWORD) " \
                        "VALUES (?, ?);";

    rc = sqlite3_prepare_v2(myDataBase, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot prepare statement.\n";
        return 0;
    }

    rc = sqlite3_bind_text(stmt, 1, usrname, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot bind parameter.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    char encoded[30] = "";
    encodePasswd(passwd, encoded);

    rc = sqlite3_bind_text(stmt, 2, encoded, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot bind parameter.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        std::cout<<"Cannot run querry.\n";
        sqlite3_finalize(stmt);
        return 0;    
    }

    sqlite3_finalize(stmt);
    return 1;
}

bool getGenres(char* response) {
    int rc;
    sqlite3_stmt *stmt;
    const char *sql =   "SELECT NAME " \
                        "FROM GENRES;";

    rc = sqlite3_prepare_v2(myDataBase, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot prepare statement.\n";
        return 0;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        strcat(response, (const char*)sqlite3_column_text(stmt, 0));
        strcat(response, "\n");
    }

    if (rc != SQLITE_OK && rc != SQLITE_DONE) {
        std::cout<<"Cannot run query.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return 1;
}

bool getSubgenres(char* response) {
    int rc;
    sqlite3_stmt *stmt;
    const char *sql =   "SELECT NAME " \
                        "FROM SUBGENRES;";

    rc = sqlite3_prepare_v2(myDataBase, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot prepare statement.\n";
        return 0;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        strcat(response, (const char*)sqlite3_column_text(stmt, 0));
        strcat(response, "\n");
    }

    if (rc != SQLITE_OK && rc != SQLITE_DONE) {
        std::cout<<"Cannot run query.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return 1;
}

bool searchAuthor(const char* name, char* response) {
    int rc, i;
    sqlite3_stmt *stmt;
    const char *sql =   "SELECT ID, TITLE, AUTHOR, RATING, YEAR_OF_PUBLICATION, ISBN " \
                        "FROM BOOKS " \
                        "WHERE UPPER(AUTHOR) LIKE UPPER(?);";

    rc = sqlite3_prepare_v2(myDataBase, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot prepare statement.\n";
        return 0;
    }

    char wildcardInput[LGMAX] = "%";
    strcat(wildcardInput, name);
    strcat(wildcardInput, "%");

    rc = sqlite3_bind_text(stmt, 1, wildcardInput, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot bind parameter.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    strcpy(response, "Format: book ID, title, author, rating, year of publication, ISBN\n");

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        for (i = 0; i < 5; i++) {
            strcat(response, (const char*)sqlite3_column_text(stmt, i));
            strcat(response, ", ");
        }
        strcat(response, (const char*)sqlite3_column_text(stmt, i));
        strcat(response, "\n");
    }

    if (rc != SQLITE_OK && rc != SQLITE_DONE) {
        std::cout<<"Cannot run query.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return 1;
}

bool searchTitle(const char* title, char* response) {
    int rc, i;
    sqlite3_stmt *stmt;
    const char *sql =   "SELECT ID, TITLE, AUTHOR, RATING, YEAR_OF_PUBLICATION, ISBN " \
                        "FROM BOOKS " \
                        "WHERE UPPER(TITLE) LIKE UPPER(?);";

    rc = sqlite3_prepare_v2(myDataBase, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot prepare statement.\n";
        return 0;
    }

    char wildcardInput[LGMAX] = "%";
    strcat(wildcardInput, title);
    strcat(wildcardInput, "%");

    rc = sqlite3_bind_text(stmt, 1, wildcardInput, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot bind parameter.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    strcpy(response, "Format: book ID, title, author, rating, year of publication, ISBN\n");

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        for (i = 0; i < 5; i++) {
            strcat(response, (const char*)sqlite3_column_text(stmt, i));
            strcat(response, ", ");
        }
        strcat(response, (const char*)sqlite3_column_text(stmt, i));
        strcat(response, "\n");
    }

    if (rc != SQLITE_OK && rc != SQLITE_DONE) {
        std::cout<<"Cannot run query.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return 1;
}

bool searchGenre(const char* genre, char* response) {
    int rc, i;
    sqlite3_stmt *stmt;
    const char *sql =   "SELECT b.ID, b.TITLE, b.AUTHOR, b.RATING, b.YEAR_OF_PUBLICATION, b.ISBN " \
                        "FROM BOOKS b " \
                        "JOIN BOOKS_GENRES bg ON b.id = bg.book_id " \
                        "JOIN GENRES g ON bg.genre_id = g.id " \
                        "WHERE UPPER(g.name) LIKE UPPER(?);";

    rc = sqlite3_prepare_v2(myDataBase, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot prepare statement.\n";
        return 0;
    }

    rc = sqlite3_bind_text(stmt, 1, genre, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot bind parameter.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    strcpy(response, "Format: book ID, title, author, rating, year of publication, ISBN\n");

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        for (i = 0; i < 5; i++) {
            strcat(response, (const char*)sqlite3_column_text(stmt, i));
            strcat(response, ", ");
        }
        strcat(response, (const char*)sqlite3_column_text(stmt, i));
        strcat(response, "\n");
    }

    if (rc != SQLITE_OK && rc != SQLITE_DONE) {
        std::cout<<"Cannot run query.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return 1;
}

bool searchSubgenre(const char* subgenre, char* response) {
    int rc, i;
    sqlite3_stmt *stmt;
    const char *sql =   "SELECT b.ID, b.TITLE, b.AUTHOR, b.RATING, b.YEAR_OF_PUBLICATION, b.ISBN " \
                        "FROM BOOKS b " \
                        "JOIN BOOKS_SUBGENRES bg ON b.id = bg.book_id " \
                        "JOIN SUBGENRES g ON bg.subgenre_id = g.id " \
                        "WHERE UPPER(g.name) LIKE UPPER(?);";

    rc = sqlite3_prepare_v2(myDataBase, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot prepare statement.\n";
        return 0;
    }

    rc = sqlite3_bind_text(stmt, 1, subgenre, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot bind parameter.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    strcpy(response, "Format: book ID, title, author, rating, year of publication, ISBN\n");

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        for (i = 0; i < 5; i++) {
            strcat(response, (const char*)sqlite3_column_text(stmt, i));
            strcat(response, ", ");
        }
        strcat(response, (const char*)sqlite3_column_text(stmt, i));
        strcat(response, "\n");
    }

    if (rc != SQLITE_OK && rc != SQLITE_DONE) {
        std::cout<<"Cannot run query.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return 1;
}

bool searchISBN(const char* isbn, char* response) {
    int rc, i;
    sqlite3_stmt *stmt;
    const char *sql =   "SELECT ID, TITLE, AUTHOR, RATING, YEAR_OF_PUBLICATION, ISBN " \
                        "FROM BOOKS " \
                        "WHERE UPPER(ISBN) LIKE UPPER(?);";

    rc = sqlite3_prepare_v2(myDataBase, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot prepare statement.\n";
        return 0;
    }

    rc = sqlite3_bind_text(stmt, 1, isbn, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot bind parameter.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    strcpy(response, "Format: book ID, title, author, rating, year of publication, ISBN\n");

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        for (i = 0; i < 5; i++) {
            strcat(response, (const char*)sqlite3_column_text(stmt, i));
            strcat(response, ", ");
        }
        strcat(response, (const char*)sqlite3_column_text(stmt, i));
        strcat(response, "\n");
    }

    if (rc != SQLITE_OK && rc != SQLITE_DONE) {
        std::cout<<"Cannot run query.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return 1;
}

bool searchYear(const char* year, char* response) {
    int rc, i;
    sqlite3_stmt *stmt;
    const char *sql =   "SELECT ID, TITLE, AUTHOR, RATING, YEAR_OF_PUBLICATION, ISBN " \
                        "FROM BOOKS " \
                        "WHERE YEAR_OF_PUBLICATION = ?;";

    rc = sqlite3_prepare_v2(myDataBase, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot prepare statement.\n";
        return 0;
    }

    rc = sqlite3_bind_int(stmt, 1, atoi(year));
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot bind parameter.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    strcpy(response, "Format: book ID, title, author, rating, year of publication, ISBN\n");

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        for (i = 0; i < 5; i++) {
            strcat(response, (const char*)sqlite3_column_text(stmt, i));
            strcat(response, ", ");
        }
        strcat(response, (const char*)sqlite3_column_text(stmt, i));
        strcat(response, "\n");
    }

    if (rc != SQLITE_OK && rc != SQLITE_DONE) {
        std::cout<<"Cannot run query.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return 1;
}

bool getAuthors(char* response) {
    int rc;
    sqlite3_stmt *stmt;
    const char *sql =   "SELECT DISTINCT AUTHOR " \
                        "FROM BOOKS;";

    rc = sqlite3_prepare_v2(myDataBase, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot prepare statement.\n";
        return 0;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        strcat(response, (const char*)sqlite3_column_text(stmt, 0));
        strcat(response, "\n");
    }

    if (rc != SQLITE_OK && rc != SQLITE_DONE) {
        std::cout<<"Cannot run query.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return 1;   
}

bool getBookPath(int bookId, char* path) {
    int rc;
    sqlite3_stmt *stmt;
    const char *sql =   "SELECT CONTENT_PATH " \
                        "FROM BOOKS " \
                        "WHERE ID = ?;";

    rc = sqlite3_prepare_v2(myDataBase, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot prepare statement.\n";
        return 0;
    }

    rc = sqlite3_bind_int(stmt, 1, bookId);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot bind parameter.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_OK || rc == SQLITE_DONE) {
        path[0] = 0;
        sqlite3_finalize(stmt);
        return 1;
    }

    if (rc != SQLITE_ROW) {
        std::cout<<"Cannot run query.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    strcpy(path, (const char*)sqlite3_column_text(stmt, 0));

    sqlite3_finalize(stmt);
    return 1;   
}

bool getBooks(vector<string> &recs) {
    int rc, i;
    sqlite3_stmt *stmt;
    const char *sql =   "SELECT ID, TITLE, AUTHOR, RATING, YEAR_OF_PUBLICATION, ISBN " \
                        "FROM BOOKS;";

    rc = sqlite3_prepare_v2(myDataBase, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout<<"Cannot prepare statement.\n";
        return 0;
    }

    char aux[LGMAX] = "";

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        strcpy(aux, (const char*)sqlite3_column_text(stmt, 0));

        for (i = 1; i < 6; i++) {
            strcat(aux, ", ");
            strcat(aux, (const char*)sqlite3_column_text(stmt, i));
        }
        strcat(aux, "\n");
        recs.push_back(string(aux));
    }

    if (rc != SQLITE_OK && rc != SQLITE_DONE) {
        std::cout<<"Cannot run query.\n";
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return 1;   
}