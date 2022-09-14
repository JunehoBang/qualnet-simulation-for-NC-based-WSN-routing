#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits>
#include <list>
#include "routing_wsnselectedcoder.h"
#include "network_ip.h"
#include "api.h"
#include "advertising_wsn_neighborinfo_diffusion.h"
#include "partition.h"

#include "routing_wsnflooding.h"
#include "routing_wsnforwarding.h"
#include "mapping.h"
#include "clock.h"




const MaxMatch::Layer MaxMatch::InfLayer(std::numeric_limits<MaxMatch::Layer>::max());

MaxMatch::Edge::Edge()
	: u_vertex(std::numeric_limits<VertexIndex>::min()),
	v_vertex(std::numeric_limits<VertexIndex>::min()),
	idx(std::numeric_limits<EdgeIndex>::min())
{
}

MaxMatch::Edge::Edge(const VertexIndex& u_vertex_, const VertexIndex& v_vertex_, const EdgeIndex& idx_)
	: u_vertex(u_vertex_),
	v_vertex(v_vertex_),
	idx(idx_)
{
}

MaxMatch::Vertex::Vertex()
	: idx(std::numeric_limits<VertexIndex>::min())
{
}

MaxMatch::Vertex::Vertex(const VertexType& type_, const std::string& name_, const VertexIndex& idx_)
	: type(type_),
	name(name_),
	idx(idx_)
{
}

MaxMatch::MaxMatch()
{
	addVertex(U_Vertex, "NILL U Vertex");
	addVertex(V_Vertex, "NILL V Vertex");
}

const MaxMatch::Vertexes& MaxMatch::u_vertexes() const
{
	return m_u_vertexes;
}

const MaxMatch::Vertexes& MaxMatch::v_vertexes() const
{
	return m_v_vertexes;
}

const MaxMatch::Edges& MaxMatch::edges() const
{
	return m_edges;
}

void MaxMatch::addVertex(const VertexType& type, const std::string& name)
{
	VertexNamesToIndexes& namesToIdxs(type == U_Vertex ? m_u_vertNamesToIdxs : m_v_vertNamesToIdxs);
	Vertexes& vertexes(type == U_Vertex ? m_u_vertexes : m_v_vertexes);

	VertexNamesToIndexes::iterator nameToIdx(namesToIdxs.find(name));
	if(nameToIdx != namesToIdxs.end())
	{
		return;
		/*char partname(type == U_Vertex ? 'u' : 'v');*/
		/*	throw std::string("MaxMatch::addVertex(const VertexType& type, const string& name): A ") + partname +
		" vertex already exists with the specified name.";*/
	}

	VertexIndex idx(vertexes.size());
	vertexes.push_back(Vertex(type, name, idx));
	namesToIdxs[name] = idx;
}

void MaxMatch::addEdge(const std::string& u_vertexName, const std::string& v_vertexName)
{
	VertexNamesToIndexes::iterator nameToIdx(m_u_vertNamesToIdxs.find(u_vertexName));
	if(nameToIdx == m_u_vertNamesToIdxs.end())
	{
		throw std::string("MaxMatch::addEdge(const string& u_vertexName, const string& v_vertexName): "
			"no u vertex with u_vertexName exists.");
	}
	VertexIndex uIdx(nameToIdx->second);
	Vertex& u(m_u_vertexes[uIdx]);
	nameToIdx = m_v_vertNamesToIdxs.find(v_vertexName);
	if(nameToIdx == m_v_vertNamesToIdxs.end())
	{
		throw std::string("MaxMatch::addEdge(const string& u_vertexName, const string& v_vertexName): "
			"no v vertex with v_vertexName exists.");
	}
	VertexIndex vIdx(nameToIdx->second);
	Vertex& v(m_v_vertexes[vIdx]);

	EdgeIndex edgeIdx(m_edges.size());
	m_edges.push_back(Edge(uIdx, vIdx, edgeIdx));
	u.edges.push_back(edgeIdx);
	v.edges.push_back(edgeIdx);
}

const MaxMatch::VertexIndexes& MaxMatch::us_to_vs() const
{
	return m_us_to_vs;
}

const MaxMatch::VertexIndexes& MaxMatch::vs_to_us() const
{
	return m_vs_to_us;
}

int MaxMatch::hopcoftKarp()
{
	int matches(0);

	m_layers.resize(m_u_vertexes.size());

	m_us_to_vs.resize(m_u_vertexes.size());
	std::fill(m_us_to_vs.begin(), m_us_to_vs.end(), NillVertIdx);

	m_vs_to_us.resize(m_v_vertexes.size());
	std::fill(m_vs_to_us.begin(), m_vs_to_us.end(), NillVertIdx);

	VertexIndex uIdx, uIdxEnd(m_u_vertexes.size());
	while(makeLayers())
	{
		for(uIdx = 1; uIdx < uIdxEnd; ++uIdx)
		{
			if(m_us_to_vs[uIdx] == NillVertIdx)
			{
				if(findPath(uIdx))
				{
					++matches;
				}
			}
		}
	}

	return matches;
}

bool MaxMatch::makeLayers()
{
	std::list<VertexIndex> searchQ;
	VertexIndex uIdx, uIdxEnd(m_u_vertexes.size()), nextUIdx, vIdx;
	// Put all free u vertexes in layer 0 and queue them for searching
	for(uIdx = 1; uIdx < uIdxEnd; ++uIdx)
	{
		if(m_us_to_vs[uIdx] == NillVertIdx)
		{
			m_layers[uIdx] = 0;
			searchQ.push_front(uIdx);
		}
		else
		{
			m_layers[uIdx] = InfLayer;
		}
	}
	m_layers[NillVertIdx] = InfLayer;
	EdgeIndexes::const_iterator edgeIdx;
	while(!searchQ.empty())
	{
		uIdx = searchQ.back();
		searchQ.pop_back();
		EdgeIndexes& edges(m_u_vertexes[uIdx].edges);
		for(edgeIdx = edges.cbegin(); edgeIdx != edges.cend(); ++edgeIdx)
		{
			vIdx = m_edges[*edgeIdx].v_vertex;
			nextUIdx = m_vs_to_us[vIdx];
			Layer& nextULayer(m_layers[nextUIdx]);
			if(nextULayer == InfLayer)
			{
				nextULayer = m_layers[uIdx] + 1;
				searchQ.push_front(nextUIdx);
			}
		}
	}
	// If an augmenting path exists, m_layers[NillVertexIdx] represents the depth of the findPath DFS when the u vertex
	// at the end of an augmenting path is found.  Otherwise, this value is InfLayer.
	return m_layers[NillVertIdx] != InfLayer;
}

bool MaxMatch::findPath(const VertexIndex& uIdx)
{
	if(uIdx == NillVertIdx)
	{
		return true;
	}
	else
	{
		VertexIndex vIdx;
		Layer nextULayer(m_layers[uIdx] + 1);
		EdgeIndexes& edges(m_u_vertexes[uIdx].edges);
		for(EdgeIndexes::const_iterator edgeIdx(edges.begin()); edgeIdx != edges.end(); ++edgeIdx)
		{
			vIdx = m_edges[*edgeIdx].v_vertex;
			VertexIndex& nextUIdx(m_vs_to_us[vIdx]);
			if(m_layers[nextUIdx] == nextULayer)
			{
				if(findPath(nextUIdx))
				{
					// This edge belongs to an augmenting path - add it to the matching set
					nextUIdx = uIdx;
					m_us_to_vs[uIdx] = vIdx;
					return true;
				}
			}
		}
		// m_u_vertexes[uIdx] does not belong to an augmenting path and need not be searched further during the DFS
		// phase
		m_layers[uIdx] = InfLayer;
		return false;
	}
}
