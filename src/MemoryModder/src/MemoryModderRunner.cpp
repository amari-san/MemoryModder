/*
    > Console window manager file for MemoryModder
*/

// TODO: Maybe for fun, add memory corruption, i need a vm tho

#include "Types.hpp"
#include "Convert.hpp"
#include "Console.hpp"
#include "Table.hpp"

#include "MemoryModder.hpp"

void GetMemoryUsage(SizeT& used, SizeT& total) {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc));
    used = pmc.PrivateUsage;

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    total = memInfo.ullTotalPageFile;
}

void DisplayMemoryUsage() {
    SizeT used, total;
    GetMemoryUsage(used, total);

    Float32 amount = Float32(used) / Float32(total);

    SizeT const mega = 1000000;

    Console::SetTextStyle(FOREGROUND_RED | FOREGROUND_GREEN);
    Console::WriteLine("Memory: " + ToString<Int32>(Int32(amount * 100.0F)) + "% (" + ToString<SizeT>(used / mega) + " / " + ToString<SizeT>(total / mega) + " MB)");
    Console::ResetTextStyle();
}

void ConsoleWriteInvalidInput() {
    Console::SetTextStyle(FOREGROUND_RED | FOREGROUND_INTENSITY);
    Console::WriteLine("Invalid input.");
    Console::ResetTextStyle();
}

Boolean ConsoleAskYesNoQuestion(String const& question, Boolean hasDefaultValue = false, Boolean defaultValue = false, UInt16 questionStyle = FOREGROUND_INTENSITY) {
    while(true) {
        Console::SetTextStyle(questionStyle);
        Console::Write(question);
        Console::SetTextStyle(FOREGROUND_INTENSITY);
        Console::Write(" [");
        Console::SetTextStyle(FOREGROUND_GREEN);
        Console::Write((hasDefaultValue && !defaultValue) ? "y" : "Y");
        Console::SetTextStyle(FOREGROUND_INTENSITY);
        Console::Write("/");
        Console::SetTextStyle(FOREGROUND_RED);
        Console::Write((hasDefaultValue && defaultValue) ? "n" : "N");
        Console::SetTextStyle(FOREGROUND_INTENSITY);
        Console::Write("]: ");
        Console::ResetTextStyle();
        
        String choice = Console::ReadLine();

        // Sanitize input string
        if(choice.length() == static_cast<SizeT>(1)) {
            if((choice[0] == 'N') || (choice[0] == 'n')) {
                return false;
            }
            else if((choice[0] == 'Y') || (choice[0] == 'y')) {
                return true;
            }
        }
        else if(hasDefaultValue && (choice.length() == static_cast<SizeT>(0))) {
            return defaultValue;
        }

        ConsoleWriteInvalidInput();
    }
}

Table ProcessSelectionCreateProcessesTable(std::vector<Process>& processes) {
    std::vector<TableColumn> columns = std::vector<TableColumn>();
    columns.push_back(TableColumn(" ", FOREGROUND_INTENSITY, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, 0));
    columns.push_back(TableColumn("#", FOREGROUND_INTENSITY, FOREGROUND_INTENSITY, 0));
    columns.push_back(TableColumn("ID", FOREGROUND_INTENSITY, FOREGROUND_BLUE | FOREGROUND_INTENSITY, 0));
    columns.push_back(TableColumn("Name", FOREGROUND_INTENSITY, FOREGROUND_GREEN | FOREGROUND_BLUE, 0));

    std::vector<std::vector<String>> rows = std::vector<std::vector<String>>();

    SizeT index = 0;
    for(Process const& process : processes) {
        std::vector<String> row = std::vector<String>();
        row.push_back(" ");
        row.push_back(ToString<SizeT>(index));
        row.push_back(ToString<DWORD>(process.GetId()));
        row.push_back(process.GetName());
        rows.push_back(row);

        ++index;
    }

    return Table(columns, rows);
}

Process* BeginProcessSelection(std::vector<Process>& processes) {
    Table table = ProcessSelectionCreateProcessesTable(processes);

    // Dynamic process selection
    SizeT position = 0;
    SizeT viewPosition = 0;
    SizeT viewSize = 20;
    table.rows[position][0] = ">";
    Boolean render = true; // Render first frame
    while(true) {
        // Update
        if(render) {
            Console::Clear();
            Console::SetTextStyle(FOREGROUND_INTENSITY);
            Console::WriteLine("Processes:");
            Console::WriteLine("([Down/Up Arrow] to scroll, [Enter] to select, [R] to refresh)");
            Console::WriteLine();
            WriteTable(table, true, viewPosition, viewSize, FOREGROUND_INTENSITY);
            DisplayMemoryUsage();
        }

        render = false;

        std::vector<ConsoleKeyEvent> keyEvents = Console::WaitKeyEvents();
        for(ConsoleKeyEvent const& keyEvent : keyEvents) {
            if(keyEvent.down) {
                if(keyEvent.key == ConsoleKey::ArrowDown) {
                    table.rows[position][0] = " ";
                    position = min(position + 1, table.rows.size() - 1);
                    table.rows[position][0] = ">";

                    viewPosition = max(viewPosition, max(position, viewSize - 1) - viewSize + 1);
                    
                    render = true;
                }
                else if(keyEvent.key == ConsoleKey::ArrowUp) {
                    table.rows[position][0] = " ";
                    position = (position == 0) ? 0 : position - 1;
                    table.rows[position][0] = ">";

                    viewPosition = min(viewPosition, position);

                    render = true;
                }
                else if(keyEvent.key == ConsoleKey::Return) {
                    return &processes[position];
                }
                else if(keyEvent.key == ConsoleKey::R) {
                    return nullptr;
                }
            }
        }

        Sleep(1000 / 60);
    }
}

template<typename T>
void BeginMemoryModdingWriteProcess(MemoryModder& modder) {
    SizeT address;
    while(true) {
        Console::SetTextStyle(FOREGROUND_INTENSITY);
        Console::Write("Address [<Address>]: ");
        Console::Write("0x");
        Console::SetTextStyle(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        try {
            Console::SetTextStyle(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            address = FromString<SizeT>(Console::ReadLine(), std::ios_base::hex);
            Console::ResetTextStyle();
            break;
        }
        catch(Int8) {
            ConsoleWriteInvalidInput();
        }
    }

    T value;
    while(true) {
        Console::SetTextStyle(FOREGROUND_INTENSITY);
        Console::Write(String("Value [<") + GetTypeName<T>() + ">]: ");
        try {
            Console::SetTextStyle(FOREGROUND_GREEN | FOREGROUND_BLUE);
            value = FromString<T>(Console::ReadLine());
            Console::ResetTextStyle();
            break;
        }
        catch(Int8) {
            ConsoleWriteInvalidInput();
        }
    }

    try {
        modder.Write<T>(address, value);

        Console::SetTextStyle(FOREGROUND_INTENSITY);
        Console::Write("Wrote ");
        Console::SetTextStyle(FOREGROUND_GREEN | FOREGROUND_BLUE);
        Console::Write(ToString<T>(value));
        Console::SetTextStyle(FOREGROUND_INTENSITY);
        Console::Write(" at ");
        Console::SetTextStyle(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        Console::Write("0x" + ToString<SizeT>(address, std::ios_base::uppercase | std::ios_base::hex));
        Console::SetTextStyle(FOREGROUND_INTENSITY);
        Console::WriteLine("!");
        Console::ResetTextStyle();
    }
    catch(Int8) {
        Console::ErrorLine("Failed to write, address is out of accessible process range.");
    }
}

template<typename T>
Table MemoryModdingFindCreateAddressesTable(MemoryModder const& modder, MemoryList<T> const& data, SizeT count) {
    std::vector<TableColumn> columns = std::vector<TableColumn>();
    columns.push_back(TableColumn("#", FOREGROUND_INTENSITY, FOREGROUND_INTENSITY, 0));
    columns.push_back(TableColumn("Address", FOREGROUND_INTENSITY, FOREGROUND_BLUE | FOREGROUND_INTENSITY, 0));
    columns.push_back(TableColumn("Value", FOREGROUND_INTENSITY, FOREGROUND_GREEN | FOREGROUND_BLUE, 0));

    std::vector<std::vector<String>> rows = std::vector<std::vector<String>>();

    std::vector<SizeT> const addresses = data.GetFirstAddresses(count);
    SizeT size = addresses.size();
    SizeT sizeAll = data.GetSize();

    for(SizeT index = 0; index < size; ++index) {
        SizeT address = addresses[index];

        std::vector<String> row = std::vector<String>();
        row.push_back(ToString<SizeT>(index));
        row.push_back("0x" + ToString<SizeT>(address, std::ios_base::uppercase | std::ios_base::hex));

        try {
            T value = modder.Read<T>(address);
            row.push_back(ToString<T>(value));
        }
        catch(Int8) {
            row.push_back("???");
        }

        rows.push_back(row);
    }
    return Table(columns, rows);
}

template<typename T>
void MemoryModdingFindWriteAddresses(MemoryModder const& modder, MemoryList<T> const& data, SizeT count) {
    {
        // Statistics
        Console::SetTextStyle(FOREGROUND_INTENSITY);
        Console::WriteLine("-- " + modder.GetProcessName() + " --");
        Console::WriteLine();

        SizeT size = data.GetSize();
        SizeT sizeReal = size * data.GetStride();
        Float32 fragmented = data.GetFragmentation();

        Console::WriteLine("List statistics:");
        Console::WriteLine("|-Addresses: " + ToString<SizeT>(size));
        Console::WriteLine("|-Size: " + ToString<SizeT>(sizeReal) + " B / " + ToString<SizeT>(sizeReal / 1000) + " KB / " + ToString<SizeT>(sizeReal / 1000000) + " MB");
        Console::WriteLine("|-Fragmented: " + ToString<Int32>(static_cast<Int32>(fragmented * 100.0F)) + "%");

        Console::SetTextStyle(FOREGROUND_INTENSITY);
        Console::WriteLine(String(GetTypeName<T>()) + " is " + ToString<SizeT>(GetBitSize<T>()) + " bit(s) long");
        Console::ResetTextStyle();

        DisplayMemoryUsage();

        Console::WriteLine();
    }

    Table table = MemoryModdingFindCreateAddressesTable(modder, data, count);
    WriteTable(table, false, 0, 0, FOREGROUND_INTENSITY);

    std::vector<SizeT> const addresses = data.GetFirstAddresses(count);
    SizeT size = addresses.size();
    SizeT sizeAll = data.GetSize();
    SizeT sizeRest = (sizeAll - size);
    if(sizeRest > 0) {
        Console::SetTextStyle(FOREGROUND_INTENSITY);
        Console::WriteLine("...And " + ToString<SizeT>(sizeRest) + " more...");
        Console::ResetTextStyle();
    }

    Console::WriteLine();
}

template<typename T>
void BeginMemoryModdingFindProcess(MemoryModder& modder) {
    MemoryList<T> data = modder.CreateList<T>();

    SizeT sizeLast = data.GetSize();

    while(true) {
        Console::Clear();

        SizeT sizeCurrent = data.GetSize();
        Console::SetTextStyle(FOREGROUND_INTENSITY);
        Console::WriteLine("-" + ToString<SizeT>(sizeLast - sizeCurrent) + " from previous snapshot.");
        Console::ResetTextStyle();
        sizeLast = sizeCurrent;

        MemoryModdingFindWriteAddresses<T>(modder, data, 16);

        if(!ConsoleAskYesNoQuestion("Filter?", true, true)) {
            break;
        }

        MemoryComparison comparison;
        while(true) {
            Console::SetTextStyle(FOREGROUND_INTENSITY);
            Console::Write("Comparison [");
            Console::SetTextStyle(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            Console::Write("==");
            Console::SetTextStyle(FOREGROUND_INTENSITY);
            Console::Write(",!=,<,>,<=,>=]: ");
            Console::SetTextStyle(FOREGROUND_INTENSITY);
            String comparisonString = Console::ReadLine();

            if((comparisonString == "==") || (comparisonString == "")) {
                comparison = MemoryComparison::Equals;
                break;
            }
            else if(comparisonString == "!=") {
                comparison = MemoryComparison::NotEquals;
                break;
            }
            else if(comparisonString == "<") {
                comparison = MemoryComparison::LessThan;
                break;
            }
            else if(comparisonString == ">") {
                comparison = MemoryComparison::GreaterThan;
                break;
            }
            else if(comparisonString == "<=") {
                comparison = MemoryComparison::LessThanEquals;
                break;
            }
            else if(comparisonString == ">=") {
                comparison = MemoryComparison::GreaterThanEquals;
                break;
            }

            ConsoleWriteInvalidInput();
        }

        T value;
        while(true) {
            Console::SetTextStyle(FOREGROUND_INTENSITY);
            Console::Write(String("Value [<") + GetTypeName<T>() + ">]: ");
            try {
                Console::SetTextStyle(FOREGROUND_GREEN | FOREGROUND_BLUE);
                value = FromString<T>(Console::ReadLine());
                break;
            }
            catch(Int8) {
                ConsoleWriteInvalidInput();
            }
        }

        Console::SetTextStyle(FOREGROUND_INTENSITY);
        Console::WriteLine("...");

        data = modder.FilterList<T>(data, value, comparison);

        Console::ResetTextStyle();
    }
}

template<typename T>
void BeginMemoryModdingProcess(MemoryModder& modder) {
    while(true) {
        Console::Clear();
        Console::SetTextStyle(FOREGROUND_INTENSITY);
        Console::Write(String("Memory: Modding: (<") + GetTypeName<T>() + ">) [back, write, find]: ");
        Console::ResetTextStyle();
        String task = Console::ReadLine();

        if(task == "back") {
            return;
        }
        else if(task == "write") {
            BeginMemoryModdingWriteProcess<T>(modder);
        }
        else if(task == "find") {
            BeginMemoryModdingFindProcess<T>(modder);
        }
        else {
            ConsoleWriteInvalidInput();
        }
    }
}

void BeginMemoryModdingTypeOptions(MemoryModder& modder) {
    while(true) {
        Console::Clear();
        Console::SetTextStyle(FOREGROUND_INTENSITY);
        Console::Write("Memory: Modding: Type: [back, int8, int16, int32, int64, uint8, uint16, uint32, uint64, float32, float64]: ");
        Console::ResetTextStyle();
        String typeString = Console::ReadLine();

        if(typeString == "back") {
            return;
        }
        else if(typeString == "int8") {
            BeginMemoryModdingProcess<Int8>(modder);
        }
        else if(typeString == "int16") {
            BeginMemoryModdingProcess<Int16>(modder);
        }
        else if(typeString == "int32") {
            BeginMemoryModdingProcess<Int32>(modder);
        }
        else if(typeString == "int64") {
            BeginMemoryModdingProcess<Int64>(modder);
        }
        else if(typeString == "uint8") {
            BeginMemoryModdingProcess<UInt8>(modder);
        }
        else if(typeString == "uint16") {
            BeginMemoryModdingProcess<UInt16>(modder);
        }
        else if(typeString == "uint32") {
            BeginMemoryModdingProcess<UInt32>(modder);
        }
        else if(typeString == "uint64") {
            BeginMemoryModdingProcess<UInt64>(modder);
        }
        else if(typeString == "float32") {
            BeginMemoryModdingProcess<Float32>(modder);
        }
        else if(typeString == "float64") {
            BeginMemoryModdingProcess<Float64>(modder);
        }
        else {
            ConsoleWriteInvalidInput();
        }
    }
}

void BeginProcessOptions(MemoryModder& modder) {
    while(true) {
        Console::Clear();
        Console::SetTextStyle(FOREGROUND_INTENSITY);
        Console::Write("Options: [back, mod]: ");
        Console::ResetTextStyle();
        String task = Console::ReadLine();

        if(task == "back") {
            return;
        }
        else if(task == "mod") {
            BeginMemoryModdingTypeOptions(modder);
        }
        //else if(task == "corrupt") {
            // Add warnings and confirmation inputs, e.g. vm is highly recommended
            //BeginMemoryCorruption(modder);
        //}
        else {
            ConsoleWriteInvalidInput();
        }
    }
}

int main() {
    Console::SetSize(1000, 600);

    while(true) {
        std::vector<Process> processes;

        Process* process = nullptr;

        while(true) {
            processes = GetAllProcesses();
            process = BeginProcessSelection(processes);
            if(process != nullptr) {
                break;
            }
        }

        DWORD processId = process->GetId();
        String processName = process->GetName();

        try {
            MemoryModder modder = MemoryModder(processId);

            BeginProcessOptions(modder);
        }
        catch(Int8 e) {
            if(e == 1) {
                Console::ErrorLine("This process cannot be used.");
            }
            else if(e == 2) {
                Console::ErrorLine("This process has stopped running.");
            }
        }
    }
}
