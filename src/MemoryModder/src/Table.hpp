#pragma once

#include "Types.hpp"
#include "Console.hpp"

#include "StringUtils.hpp"

#include <vector>

#pragma warning(push)
#pragma warning(disable: 6001) // Using uninitialized memory '*object'
#pragma warning(disable: 6385) // Reading invalid data from 'object'

struct TableColumn {
public:
    TableColumn(String name, UInt16 nameColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, UInt16 elementsColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, UInt32 width = 0) {
        _name = name;
        _nameColor = nameColor;
        _elementsColor = elementsColor;
        _width = width;
    }

    String GetName() const {
        return _name;
    }

    UInt16 GetNameColor() const {
        return _nameColor;
    }

    UInt16 GetElementsColor() const {
        return _elementsColor;
    }

    UInt32 GetWidth() const {
        return _width;
    }

private:
    String _name;
    UInt16 _nameColor;
    UInt16 _elementsColor;
    UInt32 _width;
};

struct Table {
public:
    Table(std::vector<TableColumn> const& _columns, std::vector<std::vector<String>> const& _rows) {
        columns = _columns;
        rows = _rows;
    }

    std::vector<TableColumn> columns;
    std::vector<std::vector<String>> rows;
};

/// <summary>
/// <para>Possible exceptions:</para>
/// <para>(Int8)1: Columns count mismatch in one of the rows provided.</para>
/// </summary>
void WriteTable(Table const& table, Boolean enableLimits = false, SizeT rowStart = 0, SizeT rowSize = 0, UInt16 gridColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE) {
#pragma region Calculation
    // Check if number of columns match in all the rows
    SizeT columnsCount = table.columns.size();
    SizeT rowsCount = table.rows.size();
    for(std::vector<String> const& row : table.rows) {
        if(columnsCount != row.size()) {
            throw (Int8)1;
        }
    }

    // Find ideal fit widths for columns if column width is 0
    UInt32* columnWidths = new UInt32[columnsCount];
    {
        SizeT columnIndex = 0;
        for(TableColumn const& column : table.columns) {
            columnWidths[columnIndex] = column.GetWidth();

            if(column.GetWidth() == 0) {
                columnWidths[columnIndex] = max(columnWidths[columnIndex], static_cast<UInt32>(column.GetName().length()));
            }

            ++columnIndex;
        }
    }

    for(std::vector<String> const& row : table.rows) {
        SizeT columnIndex = 0;
        for(String const& element : row) {
            TableColumn const& column = table.columns[columnIndex];

            if(column.GetWidth() == 0) {
                columnWidths[columnIndex] = max(columnWidths[columnIndex], static_cast<UInt32>(element.length()));
            }

            ++columnIndex;
        }
    }

    String const gridCross = "|";
    String const gridVertical = "|";
    String const gridHorizontal = "-";
    String const gridLimit = ".";

    // Calculate total width
    SizeT totalWidth = 0;
    {
        SizeT gridVerticalLength = gridVertical.length();
        for(SizeT columnIndex = 0; columnIndex < columnsCount; ++columnIndex) {
            TableColumn const& column = table.columns[columnIndex];
            UInt32 columnWidth = columnWidths[columnIndex];

            totalWidth += columnWidth;
        }
        totalWidth += gridVerticalLength * (columnsCount + 1);
    }

    SizeT rowEnd = rowStart + rowSize;
    Boolean showTopLimit = enableLimits && (rowStart > 0);
    Boolean showBottomLimit = enableLimits && (rowsCount > rowEnd);

    SizeT rowStart2 = enableLimits ? rowStart : 0;
    SizeT rowEnd2 = enableLimits ? min(rowEnd, rowsCount) : rowsCount;
#pragma endregion Calculation

#pragma region Writing
    Int16 x, y;
    Console::GetCursorPosition(x, y);
    Int16 carriageReturnX = x;

    // Column names
    
    for(SizeT columnIndex = 0; columnIndex < columnsCount; ++columnIndex) {
        TableColumn const& column = table.columns[columnIndex];
        UInt32 columnWidth = columnWidths[columnIndex];

        Console::SetCursorPosition(x, y);
        Console::SetTextStyle(gridColor);
        Console::Write("|");
        Console::SetTextStyle(column.GetNameColor());
        Console::Write(column.GetName().substr(0, static_cast<SizeT>(columnWidth)));

        x += columnWidth + static_cast<Int16>(gridVertical.length());
    }
    Console::SetCursorPosition(x, y);
    Console::SetTextStyle(gridColor);
    Console::Write("|");
    y += 1;
    x = carriageReturnX;

    // Column name seperator

    for(SizeT columnIndex = 0; columnIndex < columnsCount; ++columnIndex) {
        TableColumn const& column = table.columns[columnIndex];
        UInt32 columnWidth = columnWidths[columnIndex];

        Console::SetCursorPosition(x, y);
        Console::SetTextStyle(gridColor);
        Console::Write("|" + StringRepeat(gridHorizontal, static_cast<SizeT>(columnWidth)));

        x += columnWidth + static_cast<Int16>(gridVertical.length());
    }
    Console::SetCursorPosition(x, y);
    Console::SetTextStyle(gridColor);
    Console::Write("|");
    y += 1;
    x = carriageReturnX;

    // Column values

    if(showTopLimit) {
        Console::SetCursorPosition(x, y);
        Console::WriteLine(StringRepeat(gridLimit, totalWidth));
        y += 1;
        x = carriageReturnX;
    }

    for(SizeT rowIndex = rowStart2, s = rowEnd2; rowIndex < s; ++rowIndex) {
        std::vector<String> const& row = table.rows[rowIndex];
        SizeT columnIndex = 0;
        for(String const& element : row) {
            TableColumn const& column = table.columns[columnIndex];
            UInt32 columnWidth = columnWidths[columnIndex];

            Console::SetCursorPosition(x, y);
            Console::SetTextStyle(gridColor);
            Console::Write("|");
            Console::SetTextStyle(column.GetElementsColor());
            Console::Write(element.substr(0, static_cast<SizeT>(columnWidth)));

            x += columnWidth + static_cast<Int16>(gridVertical.length());

            ++columnIndex;
        }

        Console::SetCursorPosition(x, y);
        Console::SetTextStyle(gridColor);
        Console::Write("|");
        y += 1;
        x = carriageReturnX;
    }

    if(showBottomLimit) {
        Console::SetCursorPosition(x, y);
        Console::WriteLine(StringRepeat(gridLimit, totalWidth));
        y += 1;
        x = carriageReturnX;
    }

    Console::SetCursorPosition(x, y);
    Console::ResetTextStyle();

    delete[] columnWidths;
#pragma endregion Writing
}

#pragma warning(pop)
