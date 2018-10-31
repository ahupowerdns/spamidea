#include <string>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <regex>
#include <map>
#include <fstream>
using namespace std;

static void chomp(char* p)
{
  while(*p && *p!='\r' && *p!='\n')
    ++p;
  *p=0;
}

string flattenDomain(const std::string& input, const vector<string>& repl)
{
  for(const auto& r: repl) {
    auto pos = input.find(r);
    if(pos != string::npos) {
      return "*."+r;
    }
  }
  return input;
}

struct MailTreeNode
{
  MailTreeNode() 
  {}
  
  MailTreeNode(const std::string& name) : d_name(name)
  {}
  MailTreeNode* add(const std::string& name)
  {
    string rname = flattenDomain(name, {"sendgrid.net", "github.net", "outlook.com"});

    if(rname == d_name) {
      ++d_count;
      return this;
    }
    
    for(auto& mtn : d_children) {
      if(mtn->d_name == rname) {
        ++mtn->d_count;
        ++d_count;
        return mtn.get();
      }
    }
    d_children.emplace_back(std::make_unique<MailTreeNode>(rname));
    (*d_children.rbegin())->d_count = 1;
    ++d_count;
    return d_children.rbegin()->get();
  }

  void emitLinks(ofstream& fs, int depth=0) const
  {
    string prefix(depth, ' ');
    //    cout << prefix << '"' << d_name << "\"\n";
    fs << "\"a" << (void*) this << "\" [label=\""<< d_name << "\"]\n";
    unsigned int ccount=0;

    vector<MailTreeNode*> children;
    for(const auto& mtn : d_children) {
      children.push_back(mtn.get());
    }
    sort(children.begin(), children.end(),
         [](const auto& a, const auto& b) {
           return (b)->d_count < (a)->d_count;
         });
    
    //    reverse(children.begin() + children.size()/2, children.end());
    for(const auto& mtn : children) {
      ccount += mtn->d_count;
      if(mtn->d_count > 2) {
        cout << prefix << '"' << d_name << "\" -> \"" << mtn->d_name << "\""<< endl; 
        fs << "\"a" <<(void*)mtn  << "\" -> \"a" << (void*) this << "\" ";
        fs << " [ label = \" " << mtn->d_count<<" \"] ";
        fs << " [ weight = " << mtn->d_count<<" ]\n";

        mtn->emitLinks(fs, depth+1);
      }
    }
  }
  std::string d_name;
  unsigned int d_count{0};
  vector<std::unique_ptr<MailTreeNode>> d_children;
};

MailTreeNode g_mtn;
std::regex received_regex(R"(Received: from ([\S.]+).* by ([\S.]+))");
std::regex received_regex2(R"(Received: by ([\S.]+))");
void processLine(std::string& l, MailTreeNode*& start)
{
  //std::regex received_regex(R"(Received: from ([\S.]+) \(([^)]*)\) by ([\S.]+) \(([^)]*)\).*for <(.*@.*)>.*)");



  if(boost::starts_with(l, "Received: ")) {
    auto pos = l.find(';');
    if(pos != string::npos)
      l.resize(pos);
    // Received: from server.ds9a.nl ([127.0.0.1]) by localhost (server.ds9a.nl [127.0.0.1]) (amavisd-new, port 10024) with ESMTP id CedExUADWeuK for <ahu@ds9a.nl>

    //             their chosen name       what we think of that                       ourname    sometimes address                            destination
    // Received: from mail.ietf.org (mail.ietf.org [IPv6:2001:1900:3001:11::2c]) by server.ds9a.nl (Postfix) with ESMTPS id 30A4DAC011D for <bert@hubertnet.nl>



    std::smatch received_match;
    cout<<l<<endl;
    string fromServer, fromAddress, ourName, ourAddress, forAddress;
    
    if(std::regex_search(l, received_match, received_regex)) {
      static std::vector<pair<string*, int>> l({
        {&fromServer, 1},
            {&ourName, 2}
        });

          
      for(auto& p : l)
        *p.first = received_match[p.second];

      if(ourName != "localhost") {
        start=start->add(ourName);
      }

      cout << "\t"<<received_match[1] << " | " << received_match[2] << " | " << received_match[3]<< " | " << received_match[4] << endl;
    }
    else if(std::regex_search(l, received_match, received_regex2)) {
      ourName = received_match[1];
      start=start->add(ourName);
      cout << "\t"<<received_match[1] << " | " << received_match[2] << " | " << received_match[3]<< " | " << received_match[4] << endl;
    }
    else
      cout << "\tNo match"<<endl;
  }
}

void processReceived(FILE* fp)
{
  char line[512];
  std::string completeline;
  MailTreeNode* start = &g_mtn;
  while(fgets(line, sizeof(line)-1, fp)) {
    chomp(line);
    if(!*line)
      break;
    if(isspace(*line)) {
      *line=' ';
      completeline += line;
    }
    else {
      processLine(completeline, start);
      completeline=line;
    }
  }
}

int main(int argc, char**argv)
{
  for(int n = 1 ; n < argc; ++n) {
    FILE* fp = fopen(argv[n], "r");
    if(!fp) {
      cerr<<"Could not open " << argv[n] << " for reading"<<endl;
      return EXIT_FAILURE;
    }
    processReceived(fp);
    fclose(fp);
  }
  
  ofstream plot2("plot2.dot");
  plot2 << "digraph { " <<endl;
  g_mtn.emitLinks(plot2);
  plot2 << "}"<<endl;
}
