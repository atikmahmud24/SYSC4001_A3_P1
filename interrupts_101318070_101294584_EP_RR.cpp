/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @author Atik Mahmud
 * @author Jonathan Gitej
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include<interrupts_101318070_101294584.hpp>
#include <sys/stat.h>
#include <sys/types.h>

//Helper function to output in the ./output_files folder
std::string make_output_name(const std::string& inputName) {

    std::string folder = "output_files";
    struct stat st {};

    if (stat(folder.c_str(), &st) != 0) {
        mkdir(folder.c_str(), 0777);
    }

    std::string name = inputName;
    std::size_t slashPos = name.find_last_of("/\\");
    if (slashPos != std::string::npos) {
        name = name.substr(slashPos + 1);
    }

    std::string stem = name;
    std::size_t dotPos = stem.find_last_of('.');
    if (dotPos != std::string::npos) {
        stem = stem.substr(0, dotPos);
    }

    std::string suffix = stem;
    std::string prefix = "input_file";
    if (suffix.rfind(prefix, 0) == 0) {
        suffix = suffix.substr(prefix.size());
    } else {
        suffix.clear();
    }

    return folder + "/execution" + suffix + ".txt";
}

//Bonus
void memory(unsigned int time) {
    std::ofstream memlog("memory_status.txt", std::ios::app);
    if (!memlog.is_open()) {
        return;
    }

    memlog << "================ MEMORY STATUS ================\n";
    memlog << "Time: " << time << "\n\n";

    memlog << "Partition | Size | Occupied by\n";
    memlog << "------------------------------------------\n";

    int total_used = 0;
    int total_free = 0;

    for (const auto &part : memory_paritions) {
        memlog << part.partition_number << "         | "
               << part.size << " MB | ";

        if (part.occupied == -1) {
            memlog << "free";
            total_free += part.size;
        } else {
            memlog << "PID " << part.occupied;
            total_used += part.size;
        }

        memlog << "\n";
    }

    memlog << "\nTotal Used: " << total_used << " MB\n";
    memlog << "Total Free: " << total_free << " MB\n";
    memlog << "==============================================\n\n";
}

void FCFS(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}

void EP(std::vector<PCB> &ready_queue){
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.PID > second.PID); 
                } 
            );
}

std::vector<PCB> priority_queue(std::vector<PCB> &ready_queue) {

    std::vector<PCB> result;

    // Sort the list based on priority rule which is (lower PID = higher priority)
    EP(ready_queue);

    // Identify the element that represents the highest priority
    if (!ready_queue.empty()) {

        const int key = ready_queue.back().PID;

        // Collect all entries matching that PID
        for (const auto &p : ready_queue) {
            if (p.PID == key) {
                result.push_back(p);
            }
        }
    }

    // If nothing was selected, return an idle placeholder
    if (result.empty()) {
        PCB placeholder;
        idle_CPU(placeholder);
        result.push_back(placeholder);
    }

    return result;
}

// Added std::string for bonus mark
std::tuple<std::string, std::string> run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    PCB running;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {//check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                assign_memory(process);

                memory(current_time); //Bonus

                process.state = READY;  //Set the process state to READY
                ready_queue.insert ( ready_queue.begin() , process ); //Add the process to the ready queue
                job_list.push_back(process); //Add it to the list of processes
                execution_status += print_exec_status(current_time, process.PID, NEW, READY);
            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready queue
        
        // Scan through all tasks currently performing I/O
        for (std::size_t i = 0; i < wait_queue.size(); ) {

            PCB &pending = wait_queue[i];

            // Check whether the required I/O time has fully elapsed
            bool io_finished = (current_time - pending.start_time) >= (pending.io_duration + pending.io_freq);

            if (io_finished) {
                // Record the transition in the output log
                execution_status += print_exec_status(current_time, pending.PID, WAITING, READY);

                // Move the process back to the ready state
                pending.state = READY;
                ready_queue.push_back(pending);

                // Remove it from the waiting list since it is no longer waiting
                wait_queue.erase(wait_queue.begin() + i);

                // Do not increment i because the list has shifted
                continue;
            }

            // Only advance to the next entry if the current one remains waiting
            i++;
        }

        /////////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////
        // Scheduler executes a CPU cycle only when either
        // a process is already running or there is something ready to run.
        if (!ready_queue.empty() || running.state != NOT_ASSIGNED) {

            // Build the priority selection list
            std::vector<PCB> pqueue = priority_queue(ready_queue);
            PCB &candidate = pqueue.back();    // highest priority entry

            // Case 1: CPU is free and a process is ready to run
            if (running.state == NOT_ASSIGNED) {

                if (candidate.PID != -1) {     // valid selection found

                    running = candidate;
                    run_process(running, job_list, ready_queue, current_time);

                    execution_status +=
                        print_exec_status(current_time, running.PID, READY, RUNNING);
                }
            }

            // CPU is busy but a higher priority task arrived
            else if (pqueue.front().PID < running.PID && pqueue.front().PID != -1) {

                // Record the demotion of the current task
                execution_status +=
                    print_exec_status(current_time, running.PID, RUNNING, READY);

                running.state = READY;
                ready_queue.insert(ready_queue.begin(), running);

                // Preempt with the highest ranked task
                running = candidate;
                run_process(running, job_list, ready_queue, current_time);

                execution_status +=
                    print_exec_status(current_time, running.PID, READY, RUNNING);
            }

            running.remaining_time--;

            // Task finished
            if (running.remaining_time == 0) {

                execution_status += print_exec_status(current_time + 1, running.PID, RUNNING, TERMINATED);

                terminate_process(running, job_list);

                memory(current_time + 1); //Bonus

                idle_CPU(running);
            }

            // Task hits an I/O event
            else if (running.io_freq != 0 &&
                    ((running.processing_time - running.remaining_time) % running.io_freq) == 0) {

                running.state = WAITING;
                wait_queue.push_back(running);

                execution_status += print_exec_status(current_time + 1, running.PID, RUNNING, WAITING);

                idle_CPU(running);
            }

            // Time slice expired
            else if ((current_time - running.start_time) >= 99) {

                execution_status += print_exec_status(current_time + 1, running.PID, RUNNING, READY);

                running.state = READY;

                if (!ready_queue.empty())
                    ready_queue.insert(ready_queue.begin(), running);
                else
                    ready_queue.push_back(running);

                idle_CPU(running);
            }
        }

        // Advance the simulation clock
        current_time++;
        /////////////////////////////////////////////////////////////////

    }
    
    //Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status, "");
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    // Added the bonus variable
    auto [exec, bonus] = run_simulation(list_process);

    //Writing in the output_files folder
    std::string outName = make_output_name(file_name);
    write_output(exec, outName.c_str());

    return 0;
}