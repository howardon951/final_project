#pragma once

#include <UltraOOXX/UltraBoard.h>
#include <UltraOOXX/Wrapper/AI.h>
#include <GUI/GUIInterface.h>

#include <iostream>
#include <cassert>
#include <chrono>
#include <cstdarg>
#include <future>
#include <type_traits>
#include <typeinfo>

namespace TA
{
    class UltraOOXX
    {
    /*memebrs: two players (AIInterface*): m_p1, m_p2,
            	one GUIInterface* : gui,
                one Ultrabord : Mainboard, */
    public:
        UltraOOXX(
            std::chrono::milliseconds runtime_limit = std::chrono::milliseconds(1000)
        ):
            m_runtime_limit(runtime_limit),
            m_P1(nullptr),
            m_P2(nullptr),
            MainBoard()
        {
            gui = new ASCII;
        }

        void setPlayer1(AIInterface *ptr) { assert(checkAI(ptr)); m_P1 = ptr; }
        void setPlayer2(AIInterface *ptr) { assert(checkAI(ptr)); m_P2 = ptr; }

        void run()
        {
            gui->title();
            int round = 0;
            if( !prepareState() ) return ;

            //Todo: Play Game
            putToGui("Hello world %d\n", 123);
            updateGuiGame();

            while (!checkGameover()) {    //judge win or lose      
                round++;
                if(round%2){
                	AIInterface *first = m_P1;
                	AIInterface *second = m_P2;
                	BoardInterface::Tag tag = BoardInterface::Tag::O;
				}
				else{
					AIInterface *first = m_P2;
                	AIInterface *second = m_P1;
                	BoardInterface::Tag tag = BoardInterface::Tag::X;
				}
               
                if (!playOneRound(first, tag, second)) {  //one move
                    //the "first" AI made illegal move, lose the game!
                    putToGui(round%2?"Player1":"Player2", " lost!\n");
                    updateGuiGame();
                    exit(-1);
                }
                updateGuiGame();
            } 
        } 

   private:
        void updateGuiGame()
        {
            gui->updateGame(MainBoard);
        }

        bool playOneRound(AIInterface *user, BoardInterface::Tag tag, AIInterface *enemy)
        {               
            auto pos = call(&AIInterface::queryWhereToPut, user, MainBoard);  //make next step
			
			//check whether the pos is legal
			//1. out of range
			if(pos.first>8 || pos.first<0 || pos.second>8 || pos.second <0){
				std::cout << "pos out of range\n"; 
				return false;	
			}
			//2. already occupied
			if(MainBoard.get(pos.first,pos.second)!=BoardInterface::Tag::None){
				std::cout << "pos already occupied\n";
				return false;
			}
				
			//take the move  
			MainBoard.get(pos.first,pos.second)=tag;
			
            //tell the enemy its coordinate  (maybe?)
			enemy.callbackReportEnemy(pos.first,pos.second);   			
			return true;
        }
        
        //added (06/06)
        bool update_status(BoardInterface& t){  //if new updates, true
        
        	//1. if already has a wintag, do nothing
			if(t.getWinTag()!=BoardInterface::Tag::None) return false;
        
        	//2. check: any winner?
			//(1,1)
			if(t.state(1,1)==BoardInterface::Tag::O || t.state(1,1)==BoardInterface::Tag::X){
				//check horizontal, vertical, diagonal
				if(t.state(1,0)==t.state(1,1) && t.state(1,0)==t.state(1,2)){
        			t.setWinTag(t.state(1,1)); return true;
        		}
        		if(t.state(0,1)==t.state(1,1) && t.state(2,1)==t.state(1,1)){
        			t.setWinTag(t.state(1,1)); return true;
				}
				if(t.state(0,0)==t.state(1,1) && t.state(0,0)==t.state(2,2)){
					t.setWinTag(t.state(1,1)); return true;
				}
				if(t.state(0,2)==t.state(1,1) && t.state(0,0)==t.state(2,0)){
					t.setWinTag(t.state(1,1)); return true;
				}
			}
			//(0,0)
        	if(t.state(0,0)==BoardInterface::Tag::O || t.state(0,0)==BoardInterface::Tag::X){
				//check horizontal, vertical (diagonal already checked)
				if(t.state(0,0)==t.state(0,1) && t.state(0,0)==t.state(0,2)){
        			t.setWinTag(t.state(0,0)); return true;
        		}
        		if(t.state(0,0)==t.state(1,0) && t.state(0,0)==t.state(2,0)){
        			t.setWinTag(t.state(0,0)); return true;
				}
			}
			//(2,2)
			if(t.state(2,2)==BoardInterface::Tag::O || t.state(2,2)==BoardInterface::Tag::X){
				//only need to check vertical
				if(t.state(0,2)==t.state(1,2) && t.state(0,2)==t.state(2,2)){
					t.setWinTag(t.state(2,2)); return true;
				}
				if(t.state(2,0)==t.state(2,1) && t.state(2,0)==t.state(2,2)){
					t.setWinTag(t.state(2,2)); return true;
				}
			}
			
			//3. no winners yet!
			//   check if it's a tie (full!) (Ultraboard doesn't have full() function)
			for (int i=0;i<3;++i)
                for (int j=0;j<3;++j)
                    if (b[i][j] == Tag::None)
                    	return false
            
            //4. it is a tie (full and no winners)(update to tie)
            t.setWinTag(BoardInterface::Tag::Tie);
            return true;
		}

        bool checkGameover()   //judge win or lose
        {
            //1.update board status
            for(int i=0;i<3;i++){
            	for(int j=0;j<3;j++){
            		if(update_status(MainBoard.sub[i][j])){
            			//2. if board is updated, then update the Ultraboard
            			return update_status(MainBoard);
					}
				}
			}
			//2. no updates done, game continues
            return false;
        }

        bool prepareState()
        {
            call(&AIInterface::init, m_P1, true);
            call(&AIInterface::init, m_P2, false);
            return true;
        }

        template<typename Func ,typename... Args, 
            std::enable_if_t< std::is_void<
                    std::invoke_result_t<Func, AIInterface, Args...>
                >::value , int> = 0 >
        void call(Func func, AIInterface *ptr, Args... args)
        {
            std::future_status status;
            auto val = std::async(std::launch::async, func, ptr, args...);
            status = val.wait_for(std::chrono::milliseconds(m_runtime_limit));

            if( status != std::future_status::ready )
            {
                exit(-1);
            }
            val.get();
        }

        template<typename Func ,typename... Args, 
            std::enable_if_t< std::is_void<
                    std::invoke_result_t<Func, AIInterface, Args...>
                >::value == false, int> = 0 >
        auto call(Func func, AIInterface *ptr, Args... args)
            -> std::invoke_result_t<Func, AIInterface, Args...>
        //call(&AIInterface::queryWhereToPut, user, MainBoard);  
        {
            std::future_status status;
            auto val = std::async(std::launch::async, func, ptr, args...);
            status = val.wait_for(std::chrono::milliseconds(m_runtime_limit));

            if( status != std::future_status::ready )
            {
                exit(-1);
            }
            return val.get();
        }

        void putToGui(const char *fmt, ...)
        {
            va_list args1;
            va_start(args1, fmt);
            va_list args2;
            va_copy(args2, args1);
            std::vector<char> buf(1+std::vsnprintf(nullptr, 0, fmt, args1));
            va_end(args1);
            std::vsnprintf(buf.data(), buf.size(), fmt, args2);
            va_end(args2);

            if( buf.back() == 0 ) buf.pop_back();
            gui->appendText( std::string(buf.begin(), buf.end()) );
        }

        bool checkAI(AIInterface *ptr) 
        {
            return ptr->abi() == AI_ABI_VER;
        }

        int m_size;
        std::vector<int> m_ship_size;
        std::chrono::milliseconds m_runtime_limit;

        AIInterface *m_P1;
        AIInterface *m_P2;
        GUIInterface *gui;

        UltraBoard MainBoard;
    };
}
