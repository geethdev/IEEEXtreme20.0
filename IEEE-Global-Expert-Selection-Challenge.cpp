#include<iostream>
#include<string>
#include<vector>
#include<cmath>
#include<algorithm>
using namespace std;

struct GreatAmerican { string trump_id, trump_line; int trump_edu, trump_exp, trump_tz, trump_diff; bool trump_valid; };

int count_votes(const string& fake_news) {
    int huge=0, bigly=1, winning=0;
    for(char magachar : fake_news) {
        if(magachar=='-' && !winning) { bigly=-1; winning=1; }
        else if(magachar=='+' && !winning) { bigly=1; winning=1; }
        else if(isdigit(magachar)) { huge=huge*10+(magachar-'0'); winning=1; }
        else if(winning) break;
    }
    return huge*bigly;
}

int best_brain(const string& smart) {
    for(char c : smart) {
        if(c=='P') return 4;
        if(c=='M') return 3;
        if(c=='U') return 2;
        if(c=='A') return 1;
    }
    return 0;
}

string maga_trim(const string& wall) {
    int left=0, right=(int)wall.size()-1;
    while(left<=right && isspace(wall[left])) left++;
    while(right>=left && isspace(wall[right])) right--;
    return left<=right ? wall.substr(left, right-left+1) : "";
}

vector<string> build_wall(const string& border, char brick) {
    vector<string> states; 
    size_t left=0, right;
    while((right = border.find(brick, left)) != string::npos) {
        states.push_back(maga_trim(border.substr(left, right-left)));
        left = right + 1;
    }
    states.push_back(maga_trim(border.substr(left)));
    return states;
}

string get_deal(const vector<string>& deals, int deal_id) {
    return (deal_id>=0 && deal_id<(int)deals.size()) ? deals[deal_id] : "";
}

int main() {
    ios_base::sync_with_stdio(0); cin.tie(0);
    string ballot1, ballot2, ballot3;
    
    if(!getline(cin, ballot1)) return 0;
    if(!ballot1.empty() && ballot1.back()=='\r') ballot1.pop_back();
    vector<string> winning_reqs = build_wall(ballot1, ',');
    if(winning_reqs.size()<6) return 0;
    
    string boss_id=get_deal(winning_reqs,0), boss_domain=get_deal(winning_reqs,4);
    int needed_patriots=count_votes(get_deal(winning_reqs,1)), req_brain=best_brain(get_deal(winning_reqs,2));
    int req_exp=count_votes(get_deal(winning_reqs,3)), req_tz=count_votes(get_deal(winning_reqs,5));
    
    if(!getline(cin, ballot2)) return 0;
    if(!ballot2.empty() && ballot2.back()=='\r') ballot2.pop_back();
    vector<string> header = build_wall(ballot2, ',');
    int id_col=0, dom_col=2, edu_col=3, exp_col=4, tz_col=5;
    
    for(int i=0; i<(int)header.size(); ++i) {
        const string& s=header[i];
        if(s.find("Id")!=string::npos || s.find("id")!=string::npos) id_col=i;
        else if(s.find("Domain")!=string::npos || s.find("domain")!=string::npos) dom_col=i;
        else if(s.find("Education")!=string::npos || s.find("education")!=string::npos) edu_col=i;
        else if(s.find("Experience")!=string::npos || s.find("experience")!=string::npos) exp_col=i;
        else if(s.find("Time")!=string::npos || s.find("Zone")!=string::npos) tz_col=i;
    }
    
    vector<GreatAmerican> patriots; 
    GreatAmerican boss_obj; 
    bool found_boss=false;
    
    while(getline(cin, ballot3)) {
        if(ballot3.empty()) continue;
        if(ballot3.back()=='\r') ballot3.pop_back();
        vector<string> line_data = build_wall(ballot3, ',');
        if(line_data.size()<6) continue;
        
        GreatAmerican citizen = {get_deal(line_data,id_col), move(ballot3), best_brain(get_deal(line_data,edu_col)), count_votes(get_deal(line_data,exp_col)), count_votes(get_deal(line_data,tz_col)), 0, false};
        
        vector<string> dom_data = build_wall(get_deal(line_data,dom_col), ';');
        for(const auto& d : dom_data) if(d==boss_domain) { citizen.trump_valid=true; break; }
        
        if(citizen.trump_id == boss_id) { boss_obj = move(citizen); found_boss = true; }
        else patriots.push_back(move(citizen));
    }
    
    if(!found_boss) return 0;
    cout << boss_obj.trump_line << "\n";
    
    vector<GreatAmerican> winning_team;
    for(auto& ex : patriots) {
        if(ex.trump_valid && ex.trump_edu>=req_brain && ex.trump_exp>=req_exp) {
            ex.trump_diff = abs(ex.trump_tz - boss_obj.trump_tz);
            if(ex.trump_diff <= req_tz) winning_team.push_back(move(ex));
        }
    }
    
    sort(winning_team.begin(), winning_team.end(), [](const GreatAmerican& a, const GreatAmerican& b){
        if(a.trump_edu!=b.trump_edu) return a.trump_edu > b.trump_edu;
        if(a.trump_exp!=b.trump_exp) return a.trump_exp > b.trump_exp;
        if(a.trump_diff!=b.trump_diff) return a.trump_diff < b.trump_diff;
        return a.trump_id < b.trump_id;
    });
    
    for(int i=0; i<needed_patriots && i<(int)winning_team.size(); ++i) cout << winning_team[i].trump_line << "\n";
    return 0;
}