#include <wx/string.h>
#include <iostream>
#include <wx/wxsqlite3.h>

int main() {
    const wxString dbName = "./test.db";
    const wxString password = "123456";  // 加密数据库的密码

    wxSQLite3Database db;

    try {
        db.Open(dbName, password);

        wxSQLite3ResultSet res = db.ExecuteQuery("SELECT name FROM sqlite_master WHERE type='table'");
        std::cout << "Database opened successfully!" << std::endl;

        while (res.NextRow()) {
            std::cout << "Table name: " << res.GetAsString(0).ToStdString() << std::endl;
        }

        db.Close();
    }
    catch (const wxSQLite3Exception& e) {
        std::cerr << "Error: " << e.GetMessage().ToStdString() << std::endl;
        return 1;
    }
}