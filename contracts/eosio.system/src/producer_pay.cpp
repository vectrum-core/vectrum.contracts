#include <eosio.system/eosio.system.hpp>
#include <eosio.token/eosio.token.hpp>

namespace eosiosystem {

   using eosio::current_time_point;
   using eosio::microseconds;
   using eosio::token;

   void system_contract::onblock( ignore<block_header> ) {
      using namespace eosio;

      require_auth(get_self());

      block_timestamp timestamp;
      name producer;
      _ds >> timestamp >> producer;

      // _gstate2.last_block_num is not used anywhere in the system contract code anymore.
      // Although this field is deprecated, we will continue updating it for now until the last_block_num field
      // is eventually completely removed, at which point this line can be removed.
      _gstate2.last_block_num = timestamp;

      /** until activation, no new rewards are paid */
      if( _gstate.thresh_activated_stake_time == time_point() )
         return;

      if( _gstate.last_buckets_fill == time_point() )
         _gstate.last_buckets_fill = current_time_point();

      // эмиссия раз в минуту
      const auto ct = current_time_point();
      const auto usecs_since_last_fill = (ct - _gstate.last_buckets_fill).count();
      if( usecs_since_last_fill > useconds_per_minute && _gstate.last_buckets_fill > time_point() ) {
         const auto timePassedAfterActivation = (ct - _gstate.thresh_activated_stake_time).count();
         const double rate = get_continuous_rate(timePassedAfterActivation);
         _gstate4.continuous_rate = rate;
         const asset token_supply = token::get_supply(token_account, core_symbol().code() );
   
         double inflation = (rate * double(token_supply.amount) * double(usecs_since_last_fill)) / double(useconds_per_month);
         check( inflation <= double(std::numeric_limits<int64_t>::max() - ((1ll << 10) - 1)),
                "overflow in calculating new tokens to be issued; inflation rate is too high" );
         int64_t new_tokens = (inflation < 0.0) ? 0 : static_cast<int64_t>(inflation);

         int64_t to_bpay    = (new_tokens * uint128_t(pay_factor_precision)) / _gstate4.bpay_factor; // 10%
         int64_t to_vpay    = (new_tokens * uint128_t(pay_factor_precision)) / _gstate4.vpay_factor; // 10%
         int64_t to_upay    = new_tokens - to_bpay - to_vpay; // 80%

         if( new_tokens > 0 ) {
            {
               token::issue_action issue_act{ token_account, { {get_self(), active_permission} } };
               issue_act.send( get_self(), asset(new_tokens, core_symbol()), "issue tokens for rewards" );
            }
            {
               token::transfer_action transfer_act{ token_account, { {get_self(), active_permission} } };
               if( to_bpay > 0 ) {
                  transfer_act.send( get_self(), bpay_account, asset(to_bpay, core_symbol()), "fund a producers per-block bucket" );
               }
               if( to_vpay > 0 ) {
                  transfer_act.send( get_self(), vpay_account, asset(to_vpay, core_symbol()), "fund a producers per-vote bucket" );
               }
               if( to_upay > 0 ) {
                  transfer_act.send( get_self(), upay_account, asset(to_upay, core_symbol()), "fund a users per-vote bucket" );
               }
            }
         }

         _gstate.bpay_bucket       += to_bpay;
         _gstate.vpay_bucket       += to_vpay;
         _gstate.upay_bucket       += to_upay;
         _gstate.last_buckets_fill  = ct;
      }

      /**
       * At startup the initial producer may not be one that is registered / elected
       * and therefore there may be no producer object for them.
       */
      auto prod = _producers.find( producer.value );
      if ( prod != _producers.end() ) {
         _gstate.total_unpaid_blocks++;
         _producers.modify( prod, same_payer, [&](auto& p ) {
               p.unpaid_blocks++;
         });
      }

      /// only update block producers once every minute, block_timestamp is in half seconds
      if( timestamp.slot - _gstate.last_producer_schedule_update.slot > 120 ) {
         update_elected_producers( timestamp );

         // закрытие победителя аукциона имен раз в сутки
         if( (timestamp.slot - _gstate.last_name_close.slot) > blocks_per_day ) {
            name_bid_table bids(get_self(), get_self().value);
            auto idx = bids.get_index<"highbid"_n>();
            auto highest = idx.lower_bound( std::numeric_limits<uint64_t>::max()/2 );
            if( highest != idx.end() &&
                highest->high_bid > 0 &&
                (current_time_point() - highest->last_bid_time) > microseconds(useconds_per_day) &&
                _gstate.thresh_activated_stake_time > time_point() &&
                (current_time_point() - _gstate.thresh_activated_stake_time) > microseconds(14 * useconds_per_day)
            ) {
               _gstate.last_name_close = timestamp;
               idx.modify( highest, same_payer, [&]( auto& b ){
                  b.high_bid = -b.high_bid;
               });

               // деньги с аукциона идут в награды всем
               int64_t to_rewards = highest->high_bid;
               int64_t to_bpay    = (to_rewards * uint128_t(pay_factor_precision)) / _gstate4.bpay_factor;
               int64_t to_vpay    = (to_rewards * uint128_t(pay_factor_precision)) / _gstate4.vpay_factor;
               int64_t to_upay    = to_rewards - to_bpay - to_vpay;
               {
                  token::transfer_action transfer_act{ token_account, { {get_self(), active_permission} } };
                  if( to_bpay > 0 ) {
                     transfer_act.send( get_self(), bpay_account, asset(to_bpay, core_symbol()), "fund a producers per-block bucket" );
                  }
                  if( to_vpay > 0 ) {
                     transfer_act.send( get_self(), vpay_account, asset(to_vpay, core_symbol()), "fund a producers per-vote bucket" );
                  }
                  if( to_upay > 0 ) {
                     transfer_act.send( get_self(), upay_account, asset(to_upay, core_symbol()), "fund a users per-vote bucket" );
                  }
               }
            }
         }
      }
   }

   void system_contract::claimrewards( const name& owner ) {
      require_auth( owner );

      check( _gstate.thresh_activated_stake_time != time_point(),
                    "cannot claim rewards until the chain is activated (at least 15% of all tokens participate in voting)" );

      const auto ct = current_time_point();

      auto pro = _producers.find( owner.value );
      if ( pro != _producers.end()) { // запись есть
         const auto& prod = _producers.get( owner.value );
         if (prod.active() && ct - prod.last_claim_time > microseconds(useconds_per_hour)) {
            int64_t producer_per_block_pay = 0;
            if( _gstate.total_unpaid_blocks > 0 ) {
               producer_per_block_pay = (_gstate.bpay_bucket * prod.unpaid_blocks) / _gstate.total_unpaid_blocks;
            }

            int64_t producer_per_vote_pay = 0;
            if( _gstate.total_producer_vote_weight > 0 ) {
               producer_per_vote_pay = int64_t((_gstate.vpay_bucket * prod.total_votes) / _gstate.total_producer_vote_weight);
            }

            _gstate.bpay_bucket         -= producer_per_block_pay;
            _gstate.vpay_bucket         -= producer_per_vote_pay;
            _gstate.total_unpaid_blocks -= prod.unpaid_blocks;

            _producers.modify( prod, same_payer, [&](auto& p) {
               p.last_claim_time = ct;
               p.unpaid_blocks   = 0;
            });

            if ( producer_per_block_pay > 0 ) {
               token::transfer_action transfer_act{ token_account, { {bpay_account, active_permission}, {owner, active_permission} } };
               transfer_act.send( bpay_account, owner, asset(producer_per_block_pay, core_symbol()), "producer block pay" );
            }
            if ( producer_per_vote_pay > 0 ) {
               token::transfer_action transfer_act{ token_account, { {vpay_account, active_permission}, {owner, active_permission} } };
               transfer_act.send( vpay_account, owner, asset(producer_per_vote_pay, core_symbol()), "producer vote pay" );
            }
         }
      } // producer end

      auto vot = _voters.find( owner.value );
      if ( vot != _voters.end()) { // запись есть
         const auto& voter = _voters.get( owner.value );
         if (voter.last_personal_vote_weight > 0 && ct - voter.last_claim_time > microseconds(useconds_per_hour)) {
            int64_t user_per_vote_pay = 0;
            if( _gstate.total_user_vote_weight > 0 ) {
               user_per_vote_pay = int64_t((_gstate.upay_bucket * voter.last_personal_vote_weight) / _gstate.total_user_vote_weight);
            }

            _gstate.upay_bucket         -= user_per_vote_pay;

            _voters.modify( voter, same_payer, [&](auto& v) {
               v.last_claim_time = ct;
            });

            if ( user_per_vote_pay > 0 ) {
               token::transfer_action transfer_act{ token_account, { {upay_account, active_permission}, {owner, active_permission} } };
               transfer_act.send( upay_account, owner, asset(user_per_vote_pay, core_symbol()), "user vote pay" );
            }
         }
      } // user end
   }

}
